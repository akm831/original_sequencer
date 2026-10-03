import 'dart:math' as math;
import 'package:flutter/material.dart';
import 'native_bridge.dart';

// Presentation only: callbacks send intents to the native transport.
class SequencerPanel extends StatelessWidget {
  const SequencerPanel({super.key, required this.sequence, required this.ready,
    required this.onPlaying, required this.onBpm, required this.onStep, this.onAccent, this.accentMask = 0, this.notes, this.flags = 0, this.bass = false});

  final PrototypeSequenceState? sequence;
  final bool ready;
  final ValueChanged<int>? onAccent;
  final int accentMask;
  final List<int>? notes;
  final int flags;
  final bool bass;
  final ValueChanged<bool> onPlaying;
  final ValueChanged<double> onBpm;
  final void Function(int, bool) onStep;

  @override
  Widget build(BuildContext context) {
    final running = sequence?.running ?? false;
    final bpm = sequence?.bpm ?? 120;
    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 8, 16, 12),
        child: Column(children: [
          Row(children: [
            Expanded(child: FilledButton.icon(
              key: const Key('transport'),
              onPressed: ready ? () => onPlaying(!running) : null,
              icon: Icon(running ? Icons.stop : Icons.play_arrow),
              label: Text(running ? '停止' : '再生'),
              style: FilledButton.styleFrom(minimumSize: const Size(0, 52)),
            )),
            const SizedBox(width: 16),
            Text('${bpm.round()}', style: Theme.of(context).textTheme.headlineMedium),
            const SizedBox(width: 6),
            const Text('BPM'),
          ]),
          Slider(
            key: const Key('tempo'), min: 60, max: 240, divisions: 180,
            value: bpm, label: '${bpm.round()}',
            onChanged: ready ? onBpm : null,
          ),
          Row(children: [
            Icon(running ? Icons.graphic_eq : Icons.pause_circle_outline,
              size: 18, color: running ? Colors.deepPurple : Colors.grey),
            const SizedBox(width: 6),
            Expanded(child: Text(ready ? (running ? '再生中' : '停止中') : '音声を準備できませんでした')),
            const Text('1小節 · 16ステップ'),
          ]),
          const SizedBox(height: 12),
          Expanded(child: LayoutBuilder(builder: (context, constraints) {
            // Keep useful touch targets on short screens; only the grid scrolls.
            final width = math.min(constraints.maxWidth, 520.0);
            final height = math.max(216.0, math.min(constraints.maxHeight, 408.0));
            return SingleChildScrollView(
              child: Center(child: SizedBox(width: width, height: height,
                child: GridView.count(
                  key: const Key('step-grid'), crossAxisCount: 4,
                  physics: const NeverScrollableScrollPhysics(),
                  mainAxisSpacing: 8, crossAxisSpacing: 8,
                  childAspectRatio: (width - 24) / (height - 24),
                  children: List.generate(16, (step) {
                    final enabled = ((sequence?.stepMask ?? 0x1111) & (1 << step)) != 0;
                    final current = running && sequence?.currentStep == step;
                    return Semantics(
                      label: 'ステップ ${step + 1}${current ? '、再生位置' : ''}',
                      toggled: enabled,
                      child: GestureDetector(
                      onLongPress: ready && onAccent != null ? () => onAccent!(step) : null,
                      child: OutlinedButton(
                        key: Key('step-$step'),
                        style: OutlinedButton.styleFrom(
                          animationDuration: Duration.zero,
                          padding: EdgeInsets.zero,
                          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(14)),
                          backgroundColor: enabled ? Colors.deepPurple : Colors.grey.shade100,
                          foregroundColor: enabled ? Colors.white : Colors.black87,
                          side: BorderSide(color: current ? Colors.orange : Colors.grey.shade400,
                            width: current ? 4 : 1),
                        ),
                        onPressed: ready ? () => onStep(step, !enabled) : null,
                        child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
                          Text('${step+1}${(accentMask & (1<<step)) != 0 ? '•' : ''}',
                            style: const TextStyle(fontSize: 20,fontWeight: FontWeight.w600)),
                          if(notes!=null) Text('${noteName(notes![step])}${(flags & (1<<step)) != 0 ? ' ↗' : ''}',style: const TextStyle(fontSize: 11)),
                          if(!bass && (flags & (1<<step)) != 0) const Text('OPEN',style: TextStyle(fontSize: 10)),
                        ]),
                      ),
                    ));
                  }),
                ),
              )),
            );
          })),
          const SizedBox(height: 8),
          const Text('タップで発音ON／OFF · 長押しで編集'),
        ]),
      ),
    );
  }
}

String noteName(int note) {
  const names=['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'];
  return '${names[note%12]}${note~/12-1}';
}
