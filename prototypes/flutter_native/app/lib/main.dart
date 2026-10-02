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
        const Duration(milliseconds: 200),
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
      if (mounted) setState(() => _diagnostics = diagnostics);
    } catch (error) {
      if (mounted) setState(() => _bridgeError = error);
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
    return Scaffold(
      appBar: AppBar(title: const Text('Sequencer Prototype')),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text('Flutter + Native Audio — P3 trigger',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
            const SizedBox(height: 16),
            Text('Native bridge: ${_bridge != null ? 'loaded' : 'not loaded'}'),
            Text('Audio stream: ${d != null && d.sampleRate > 0 ? 'ready' : 'unavailable'}'),
            const SizedBox(height: 16),
            FilledButton(
              onPressed: _bridge != null && d != null && d.sampleRate > 0 ? _trigger : null,
              child: const Text('Trigger 50 ms test burst'),
            ),
            const Text('220 Hz / peak amplitude 0.08 / silence between bursts'),
            if (_triggerStatus != null) Text(_triggerStatus!),
            if (d != null) ...[
              const SizedBox(height: 16),
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
