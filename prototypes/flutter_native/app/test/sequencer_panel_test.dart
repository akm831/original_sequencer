import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:original_sequencer_flutter_prototype/native_bridge.dart';
import 'package:original_sequencer_flutter_prototype/sequencer_panel.dart';
import 'package:original_sequencer_flutter_prototype/diagnostics_screen.dart';

void main() {
  for (final profile in [
    (size: const Size(360, 640), scale: 1.0),
    (size: const Size(393, 852), scale: 1.8),
    (size: const Size(800, 1100), scale: 1.0),
  ]) {
    testWidgets('all steps and transport fit ${profile.size} at ${profile.scale}', (tester) async {
      tester.view.physicalSize = profile.size;
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.resetPhysicalSize);
      addTearDown(tester.view.resetDevicePixelRatio);
      await tester.pumpWidget(MaterialApp(
        builder: (context, child) => MediaQuery(
          data: MediaQuery.of(context).copyWith(textScaler: TextScaler.linear(profile.scale)),
          child: child!,
        ),
        home: Scaffold(appBar: AppBar(title: const Text('Sequencer')),
          body: SequencerPanel(
            sequence: const PrototypeSequenceState(120, true, 0x1111, 0, 0),
            ready: true, onPlaying: (_) {}, onBpm: (_) {}, onStep: (_, __) {},
          ),
        ),
      ));
      await tester.pump();
      expect(tester.takeException(), isNull);
      final transport = tester.getRect(find.byKey(const Key('transport')));
      expect(transport.height, greaterThanOrEqualTo(48));
      for (var i = 0; i < 16; i++) {
        final rect = tester.getRect(find.byKey(Key('step-$i')));
        expect(rect.top, greaterThanOrEqualTo(transport.bottom));
        expect(rect.bottom, lessThanOrEqualTo(profile.size.height));
        expect(rect.height, greaterThanOrEqualTo(48));
        expect(rect.width, greaterThanOrEqualTo(48));
      }
    });
  }

  testWidgets('bass notes and outgoing slide are visible and long press opens editor', (tester) async {
    int? edit;
    await tester.pumpWidget(MaterialApp(home: Scaffold(body: SequencerPanel(
      sequence:const PrototypeSequenceState(120,false,15,16,0),ready:true,
      onPlaying:(_){},onBpm:(_){},onStep:(_,__){},
      onAccent:(step)=>edit=step,notes:List.filled(16,36),flags:1,bass:true,
    ))));
    expect(find.text('C2 ↗'),findsOneWidget);
    await tester.longPress(find.byKey(const Key('step-0')));
    expect(edit,0); expect(tester.takeException(),isNull);
  });
  testWidgets('step edits and transport send the requested intents', (tester) async {
    bool? playing;
    int? step;
    bool? enabled;
    await tester.pumpWidget(MaterialApp(home: Scaffold(body: SequencerPanel(
      sequence: const PrototypeSequenceState(120, false, 0x1111, 16, 0),
      ready: true, onPlaying: (value) => playing = value, onBpm: (_) {},
      onStep: (index, value) { step = index; enabled = value; },
    ))));
    await tester.tap(find.byKey(const Key('transport')));
    expect(playing, isTrue);
    await tester.tap(find.byKey(const Key('step-0')));
    expect(step, 0);
    expect(enabled, isFalse);
    await tester.tap(find.byKey(const Key('step-1')));
    expect(step, 1);
    expect(enabled, isTrue);
  });

  testWidgets('landscape keeps transport accessible while the grid scrolls', (tester) async {
    tester.view.physicalSize = const Size(720, 320);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    await tester.pumpWidget(MaterialApp(home: Scaffold(
      appBar: AppBar(title: const Text('Sequencer')),
      body: SequencerPanel(sequence: null, ready: true,
        onPlaying: (_) {}, onBpm: (_) {}, onStep: (_, __) {}),
    )));
    expect(tester.takeException(), isNull);
    final before = tester.getRect(find.byKey(const Key('transport')));
    await tester.drag(find.byType(SingleChildScrollView), const Offset(0, -250));
    await tester.pumpAndSettle();
    expect(tester.takeException(), isNull);
    expect(tester.getRect(find.byKey(const Key('transport'))), before);
    expect(tester.getRect(find.byKey(const Key('step-15'))).bottom, lessThan(320));
  });

  testWidgets('unavailable audio disables controls', (tester) async {
    var calls = 0;
    await tester.pumpWidget(MaterialApp(home: Scaffold(body: SequencerPanel(
      sequence: null, ready: false, onPlaying: (_) => calls++,
      onBpm: (_) => calls++, onStep: (_, __) => calls++,
    ))));
    await tester.tap(find.byKey(const Key('transport')));
    await tester.tap(find.byKey(const Key('step-0')));
    expect(calls, 0);
  });

  testWidgets('diagnostics updates and test sound is blocked during playback', (tester) async {
    final d = PrototypeDiagnostics(sampleRate: 48000, callbackFrames: 96,
      callbackFramesMin: 96, callbackFramesMax: 96, renderedFrames: 100,
      callbackStartFrame: 4, callbackDurationUs: 2, callbackLoad: 0.001,
      callbackLoadP95: 0.002, callbackLoadP99: 0.003, callbackLoadPeak: 0.1,
      audioRestartCount: 0, queueDepth: 0, queueHighWaterMark: 1,
      queueOverflowCount: 0, triggerCount: 3, lastTriggerOffset: 5);
    final status = ValueNotifier(AudioStatus(d,
      const PrototypeSequenceState(120, true, 0x1111, 0, 0), null, null));
    addTearDown(status.dispose);
    var tests = 0;
    await tester.pumpWidget(MaterialApp(home: DiagnosticsScreen(
      status: status, onTest: () => tests++,
    )));
    await tester.tap(find.text('音のテスト'));
    expect(tests, 0);
    status.value = AudioStatus(d,
      const PrototypeSequenceState(120, false, 0x1111, 16, 0), null, '更新済み');
    await tester.pump();
    expect(find.text('更新済み'), findsOneWidget);
    await tester.tap(find.text('音のテスト'));
    expect(tests, 1);
  });
}
