import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'native_bridge.dart';

class AudioStatus {
  const AudioStatus(this.diagnostics, this.sequence, this.error, this.testStatus);
  final PrototypeDiagnostics? diagnostics;
  final PrototypeSequenceState? sequence;
  final Object? error;
  final String? testStatus;
}

class DiagnosticsScreen extends StatelessWidget {
  const DiagnosticsScreen({super.key, required this.status, required this.onTest});
  final ValueListenable<AudioStatus> status;
  final VoidCallback onTest;

  @override
  Widget build(BuildContext context) => Scaffold(
    appBar: AppBar(title: const Text('音声診断')),
    body: ValueListenableBuilder<AudioStatus>(
      valueListenable: status,
      builder: (context, snapshot, _) {
        final d = snapshot.diagnostics;
        final running = snapshot.sequence?.running ?? false;
        return ListView(padding: const EdgeInsets.all(20), children: [
          FilledButton.icon(
            onPressed: d != null && d.sampleRate > 0 && !running ? onTest : null,
            icon: const Icon(Icons.volume_up), label: const Text('音のテスト'),
          ),
          const SizedBox(height: 8),
          const Text('停止中のみ利用できます。220 Hzの短いテスト音を鳴らします。'),
          if (snapshot.testStatus != null) Text(snapshot.testStatus!),
          const SizedBox(height: 20),
          if (d != null) ...[
            _row('取りこぼしたステップ', '${snapshot.sequence?.missedSteps ?? 0}'),
            _row('sampleRate', '${d.sampleRate}'),
            _row('callbackFrames', '${d.callbackFrames} (min ${d.callbackFramesMin} / max ${d.callbackFramesMax})'),
            _row('renderedFrames', '${d.renderedFrames}'),
            _row('callbackStartFrame', '${d.callbackStartFrame}'),
            _row('callbackDurationUs', d.callbackDurationUs.toStringAsFixed(2)),
            _row('callbackLoad', '${(d.callbackLoad * 100).toStringAsFixed(2)}%'),
            _row('callbackLoadP95', '${(d.callbackLoadP95 * 100).toStringAsFixed(1)}%'),
            _row('callbackLoadP99', '${(d.callbackLoadP99 * 100).toStringAsFixed(1)}%'),
            _row('callbackLoadPeak', '${(d.callbackLoadPeak * 100).toStringAsFixed(2)}%'),
            _row('audioRestartCount', '${d.audioRestartCount}'),
            _row('queueDepth', '${d.queueDepth}'),
            _row('queueHighWaterMark', '${d.queueHighWaterMark}'),
            _row('queueOverflowCount', '${d.queueOverflowCount}'),
            _row('triggerCount', '${d.triggerCount}'),
            _row('lastTriggerOffset', '${d.lastTriggerOffset}'),
          ],
          if (snapshot.error != null) Text('音声エラー: ${snapshot.error}'),
        ]);
      },
    ),
  );

  Widget _row(String label, String value) => Padding(
    padding: const EdgeInsets.symmetric(vertical: 6),
    child: Wrap(spacing: 8, children: [Text('$label:'), Text(value)]),
  );
}
