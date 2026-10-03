import 'dart:async';
import 'package:flutter/material.dart';
import 'native_bridge.dart';

void main() => runApp(const PrototypeApp());

class PrototypeApp extends StatelessWidget {
  const PrototypeApp({super.key});
  @override
  Widget build(BuildContext context) => const MaterialApp(
    debugShowCheckedModeBanner: false,
    home: PrototypeScreen(),
  );
}

class PrototypeScreen extends StatefulWidget {
  const PrototypeScreen({super.key});
  @override
  State<PrototypeScreen> createState() => _PrototypeScreenState();
}

class _PrototypeScreenState extends State<PrototypeScreen> {
  PrototypeNativeBridge? _bridge;
  PrototypeDiagnostics? _diagnostics;
  PrototypeSequenceState? _sequence;
  Timer? _diagnosticsTimer;
  Object? _bridgeError;
  String? _triggerStatus;

  @override
  void initState() {
    super.initState();
    _openNativeBridge();
  }

  void _openNativeBridge() {
    try {
      final bridge = PrototypeNativeBridge.open();
      if (!bridge.startAudio()) {
        bridge.dispose();
        throw StateError('Native audio stream failed to start.');
      }
      _bridge = bridge;
      _refreshDiagnostics();
      _diagnosticsTimer = Timer.periodic(
        const Duration(milliseconds: 50),
        (_) => _refreshDiagnostics(),
      );
    } catch (error) {
      _bridgeError = error;
    }
  }

  void _refreshDiagnostics() {
    final bridge = _bridge;
    if (bridge == null) return;
    try {
      final diagnostics = bridge.diagnostics();
      final sequence = bridge.sequenceState();
      if (mounted) {
        setState(() {
          _diagnostics = diagnostics;
          _sequence = sequence;
        });
      }
    } catch (error) {
      if (mounted) setState(() => _bridgeError = error);
    }
  }

  void _control(bool Function(PrototypeNativeBridge) action) {
    final bridge = _bridge;
    if (bridge == null) return;
    try {
      if (!action(bridge)) throw StateError('Audio control unavailable.');
      _refreshDiagnostics();
    } catch (error) {
      setState(() => _bridgeError = error);
    }
  }

  void _trigger() {
    final bridge = _bridge;
    if (bridge == null) return;
    try {
      final accepted = bridge.scheduleTrigger(delayFrames: 37);
      setState(() {
        _triggerStatus = accepted
          ? 'Trigger accepted'
          : 'Trigger rejected: audio unavailable, queue full, or earlier timestamp';
      });
      _refreshDiagnostics();
    } catch (error) {
      setState(() => _bridgeError = error);
    }
  }

  @override
  void dispose() {
    _diagnosticsTimer?.cancel();
    _bridge?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final d = _diagnostics;
    final sequence = _sequence;
    final ready = _bridge != null && d != null && d.sampleRate > 0;
    final running = sequence?.running ?? false;
    return Scaffold(
      appBar: AppBar(title: const Text('Sequencer Prototype')),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text('16ステップ・シーケンサー',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            const SizedBox(height: 16),
            Text(ready ? (running ? '再生中' : '停止中 — 再生を押すとループします') : '音声を準備中'),
            const SizedBox(height: 16),
            FilledButton.icon(
              onPressed: ready ? () => _control((b) => b.setPlaying(!running)) : null,
              icon: Icon(running ? Icons.stop : Icons.play_arrow),
              label: Text(running ? '停止' : '再生'),
            ),
            const SizedBox(height: 16),
            Text('BPM ${(sequence?.bpm ?? 120).round()}'),
            Slider(
              min: 60,
              max: 240,
              divisions: 180,
              value: sequence?.bpm ?? 120,
              label: '${(sequence?.bpm ?? 120).round()}',
              onChanged: ready ? (value) => _control((b) => b.setBpm(value)) : null,
            ),
            const Text('ステップをタップして音をオン／オフ（1小節を繰り返し）'),
            const SizedBox(height: 12),
            GridView.count(
              crossAxisCount: 4,
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              mainAxisSpacing: 8,
              crossAxisSpacing: 8,
              childAspectRatio: 1.4,
              children: List.generate(16, (step) {
                final enabled = ((sequence?.stepMask ?? 0x1111) & (1 << step)) != 0;
                final current = running && sequence?.currentStep == step;
                return Semantics(
                  label: 'ステップ ${step + 1}',
                  toggled: enabled,
                  child: OutlinedButton(
                    style: OutlinedButton.styleFrom(
                      backgroundColor: enabled ? Colors.deepPurple : Colors.grey.shade100,
                      foregroundColor: enabled ? Colors.white : Colors.black87,
                      side: BorderSide(color: current ? Colors.orange : Colors.grey,
                        width: current ? 4 : 1),
                    ),
                    onPressed: ready ? () => _control((b) => b.setStep(step, !enabled)) : null,
                    child: Text('${step + 1}', style: const TextStyle(fontSize: 20)),
                  ),
                );
              }),
            ),
            const SizedBox(height: 20),
            FilledButton(
              onPressed: ready && !running ? _trigger : null,
              child: const Text('音のテスト（停止中のみ）'),
            ),
            const Text('テスト音：220 Hz、50 ms。再生中は有効なステップで鳴ります。'),
            if (_triggerStatus != null) Text(_triggerStatus!),
            if (d != null) ...[
              const SizedBox(height: 16),
              ExpansionTile(
                title: const Text('音声診断'),
                children: [
              Text('missedSteps: ${sequence?.missedSteps ?? 0}'),
              Text('sampleRate: ${d.sampleRate}'),
              Text('callbackFrames: ${d.callbackFrames} (min ${d.callbackFramesMin} / max ${d.callbackFramesMax})'),
              Text('renderedFrames: ${d.renderedFrames}'),
              Text('callbackStartFrame: ${d.callbackStartFrame}'),
              Text('callbackDurationUs: ${d.callbackDurationUs.toStringAsFixed(2)}'),
              Text('callbackLoad: ${(d.callbackLoad * 100).toStringAsFixed(2)}%'),
              Text('callbackLoadP95: ${(d.callbackLoadP95 * 100).toStringAsFixed(1)}%'),
              Text('callbackLoadP99: ${(d.callbackLoadP99 * 100).toStringAsFixed(1)}%'),
              Text('callbackLoadPeak: ${(d.callbackLoadPeak * 100).toStringAsFixed(2)}%'),
              Text('audioRestartCount: ${d.audioRestartCount}'),
              Text('queueDepth: ${d.queueDepth}'),
              Text('queueHighWaterMark: ${d.queueHighWaterMark}'),
              Text('queueOverflowCount: ${d.queueOverflowCount}'),
              Text('triggerCount: ${d.triggerCount}'),
              Text('lastTriggerOffset: ${d.lastTriggerOffset}'),
                ],
              ),
            ],
            if (_bridgeError != null) ...[
              const SizedBox(height: 16),
              Text('FFI/audio error: $_bridgeError'),
            ],
          ],
        ),
      ),
    );
  }
}
