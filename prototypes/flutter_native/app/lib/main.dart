import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'native_bridge.dart';
import 'sequencer_panel.dart';
import 'diagnostics_screen.dart';

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

class _PrototypeScreenState extends State<PrototypeScreen>
    with SingleTickerProviderStateMixin, WidgetsBindingObserver {
  PrototypeNativeBridge? _bridge;
  PrototypeDiagnostics? _diagnostics;
  PrototypeSequenceState? _sequence;
  Timer? _diagnosticsTimer;
  late final Ticker _playheadTicker;
  Object? _bridgeError;
  String? _triggerStatus;
  final _audioStatus = ValueNotifier(const AudioStatus(null, null, null, null));

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _playheadTicker = createTicker((_) => _refreshPlayhead());
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
      _publishStatus();
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
        _syncPlayheadTicker();
        _publishStatus();
      }
    } catch (error) {
      if (mounted) setState(() => _bridgeError = error);
      _publishStatus();
    }
  }

  // The native audio playhead is authoritative. Observe it before each paint;
  // this ticker never schedules notes or extrapolates the musical clock.
  void _refreshPlayhead() {
    final bridge = _bridge;
    if (bridge == null || !mounted) return;
    try {
      final next = bridge.sequenceState();
      final previous = _sequence;
      if (previous == null ||
          next.currentStep != previous.currentStep ||
          next.running != previous.running ||
          next.bpm != previous.bpm ||
          next.stepMask != previous.stepMask ||
          next.missedSteps != previous.missedSteps) {
        setState(() => _sequence = next);
      }
      _syncPlayheadTicker();
    } catch (error) {
      _playheadTicker.stop();
      setState(() => _bridgeError = error);
      _publishStatus();
    }
  }

  void _syncPlayheadTicker() {
    if (_sequence?.running ?? false) {
      if (!_playheadTicker.isActive) _playheadTicker.start();
    } else if (_playheadTicker.isActive) {
      _playheadTicker.stop();
    }
  }

  void _publishStatus() {
    _audioStatus.value = AudioStatus(_diagnostics, _sequence, _bridgeError, _triggerStatus);
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    // Leaving the app always stops transport; returning never auto-plays.
    if (state != AppLifecycleState.resumed && (_sequence?.running ?? false)) {
      _control((bridge) => bridge.setPlaying(false));
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
      _publishStatus();
    }
  }

  void _trigger() {
    final bridge = _bridge;
    if (bridge == null) return;
    try {
      final accepted = bridge.scheduleTrigger(delayFrames: 37);
      setState(() {
        _triggerStatus = accepted
          ? 'テスト音を受け付けました'
          : 'テスト音を受け付けられませんでした';
      });
      _refreshDiagnostics();
    } catch (error) {
      setState(() => _bridgeError = error);
      _publishStatus();
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    _playheadTicker.dispose();
    _audioStatus.dispose();
    _diagnosticsTimer?.cancel();
    _bridge?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final d = _diagnostics;
    final ready = _bridge != null && d != null && d.sampleRate > 0;
    return Scaffold(
      appBar: AppBar(
        title: const Text('Sequencer'),
        actions: [IconButton(
          tooltip: '音声診断', icon: const Icon(Icons.monitor_heart_outlined),
          onPressed: () => Navigator.of(context).push(MaterialPageRoute<void>(
            builder: (_) => DiagnosticsScreen(status: _audioStatus, onTest: _trigger),
          )),
        )],
      ),
      body: SequencerPanel(
        sequence: _sequence, ready: ready,
        onPlaying: (playing) => _control((b) => b.setPlaying(playing)),
        onBpm: (bpm) => _control((b) => b.setBpm(bpm)),
        onStep: (step, enabled) => _control((b) => b.setStep(step, enabled)),
      ),
    );
  }
}
