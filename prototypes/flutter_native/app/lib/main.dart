import 'dart:async';
import 'package:flutter/material.dart';
import 'native_bridge.dart';

void main() => runApp(const PrototypeApp());
class PrototypeApp extends StatelessWidget {
  const PrototypeApp({super.key});
  @override Widget build(BuildContext context) => const MaterialApp(debugShowCheckedModeBanner: false, home: PrototypeScreen());
}
class PrototypeScreen extends StatefulWidget {
  const PrototypeScreen({super.key});
  @override State<PrototypeScreen> createState() => _PrototypeScreenState();
}
class _PrototypeScreenState extends State<PrototypeScreen> {
  PrototypeNativeBridge? _bridge;
  PrototypeDiagnostics? _diagnostics;
  Timer? _diagnosticsTimer;
  Object? _bridgeError;
  bool _audioRunning = false;
  @override void initState() { super.initState(); _openNativeBridge(); }
  void _openNativeBridge() {
    try {
      final bridge = PrototypeNativeBridge.open();
      final audioRunning = bridge.startAudio();
      if (!audioRunning) { bridge.dispose(); throw StateError('Native audio stream failed to start.'); }
      _bridge = bridge; _audioRunning = true; _refreshDiagnostics();
      _diagnosticsTimer = Timer.periodic(const Duration(milliseconds: 200), (_) => _refreshDiagnostics());
    } catch (error) { _bridgeError = error; }
  }
  void _refreshDiagnostics() {
    final bridge = _bridge; if (bridge == null) return;
    try { final diagnostics = bridge.diagnostics(); if (!mounted) return; setState(() => _diagnostics = diagnostics); }
    catch (error) { if (!mounted) return; setState(() => _bridgeError = error); }
  }
  @override void dispose() { _diagnosticsTimer?.cancel(); _bridge?.dispose(); super.dispose(); }
  @override Widget build(BuildContext context) {
    final d = _diagnostics; final bridgeLoaded = _bridge != null;
    return Scaffold(appBar: AppBar(title: const Text('Sequencer Prototype')), body: Padding(padding: const EdgeInsets.all(24), child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
      const Text('Flutter + Native Audio — P2 diagnostics', style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
      const SizedBox(height: 24),
      Text('Native bridge: ${bridgeLoaded ? 'loaded' : 'not loaded'}'),
      Text('Audio stream: ${_audioRunning ? 'running' : 'stopped'}'),
      if (d != null) ...[
        const SizedBox(height: 16),
        Text('sampleRate: ${d.sampleRate}'), Text('callbackFrames: ${d.callbackFrames}'), Text('renderedFrames: ${d.renderedFrames}'),
        Text('callbackStartFrame: ${d.callbackStartFrame}'), Text('callbackDurationUs: ${d.callbackDurationUs.toStringAsFixed(2)}'),
        Text('callbackLoad: ${(d.callbackLoad * 100).toStringAsFixed(2)}%'), Text('callbackLoadPeak: ${(d.callbackLoadPeak * 100).toStringAsFixed(2)}%'),
        Text('audioRestartCount: ${d.audioRestartCount}'),
      ],
      const SizedBox(height: 16), const Text('Test tone: 220 Hz / amplitude 0.08'),
      if (_bridgeError != null) ...[const SizedBox(height: 16), Text('FFI/audio error: $_bridgeError')],
    ])));
  }
}
