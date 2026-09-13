import 'package:flutter/material.dart';

import 'native_bridge.dart';

void main() => runApp(const PrototypeApp());

class PrototypeApp extends StatelessWidget {
  const PrototypeApp({super.key});

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      debugShowCheckedModeBanner: false,
      home: PrototypeScreen(),
    );
  }
}

class PrototypeScreen extends StatefulWidget {
  const PrototypeScreen({super.key});

  @override
  State<PrototypeScreen> createState() => _PrototypeScreenState();
}

class _PrototypeScreenState extends State<PrototypeScreen> {
  PrototypeNativeBridge? _bridge;
  PrototypeDiagnostics? _diagnostics;
  Object? _bridgeError;

  @override
  void initState() {
    super.initState();
    _openNativeBridge();
  }

  void _openNativeBridge() {
    try {
      final bridge = PrototypeNativeBridge.open();
      final diagnostics = bridge.diagnostics();
      _bridge = bridge;
      _diagnostics = diagnostics;
    } catch (error) {
      _bridgeError = error;
    }
  }

  @override
  void dispose() {
    _bridge?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final diagnostics = _diagnostics;
    final bridgeLoaded = _bridge != null;

    return Scaffold(
      appBar: AppBar(title: const Text('Sequencer Prototype')),
      body: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Flutter + Native Audio — P0 skeleton',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 24),
            Text('Native bridge: ${bridgeLoaded ? 'loaded' : 'not loaded'}'),
            if (diagnostics != null) ...[
              const SizedBox(height: 16),
              Text('sampleRate: ${diagnostics.sampleRate}'),
              Text('callbackFrames: ${diagnostics.callbackFrames}'),
              Text('renderedFrames: ${diagnostics.renderedFrames}'),
              Text('audioRestartCount: ${diagnostics.audioRestartCount}'),
            ],
            if (_bridgeError != null) ...[
              const SizedBox(height: 16),
              Text('FFI error: $_bridgeError'),
            ],
          ],
        ),
      ),
    );
  }
}
