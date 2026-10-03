import 'dart:convert';
import 'package:flutter_test/flutter_test.dart';
import '../lib/groove_project.dart';
import '../lib/main.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() {
  test('all banks, accents, mute, level and tempo survive a save round trip', () {
    final original = GrooveProject.initial();
    final edited = original.copy(bpm: 173, selectedPattern: 3)
      .edit(3, 2, const GrooveTrack(0xabcd, 0x1234, 0.27, true));
    expect(GrooveProject.decode(edited.encode()).encode(), edited.encode());
    expect(original.patterns[3][2].muted, false);
    expect(() => edited.patterns[0].clear(), throwsUnsupportedError);
    expect(() => edited.patterns.clear(), throwsUnsupportedError);
  });
  test('unsupported and malformed saves are rejected rather than partially loaded', () {
    for (final mutate in <void Function(Map<String, dynamic>)>[
      (m) => m['schemaVersion'] = 3,
      (m) => m['ppqn'] = 24,
      (m) => m['bpm'] = 0,
      (m) => m['selectedPattern'] = 4,
      (m) => m['patterns'] = [],
      (m) => m['engines'] = ['unknown'],
      (m) => m['patterns'][0][0]['mask'] = 65536,
      (m) => m['patterns'][0][0]['level'] = -0.1,
      (m) => m['patterns'][0][0]['muted'] = 1,
    ]) {
      final data = jsonDecode(GrooveProject.initial().encode()) as Map<String, dynamic>;
      mutate(data);
      expect(() => GrooveProject.decode(jsonEncode(data)), throwsFormatException);
    }
    expect(() => GrooveProject.decode('{'), throwsFormatException);
  });
  test('version one migrates all edits without replacing patterns', () {
    final original = GrooveProject.initial().copy(bpm: 137, selectedPattern: 2);
    final old = jsonDecode(original.encode()) as Map<String, dynamic>;
    old['schemaVersion'] = 1;
    for(final pattern in old['patterns']) {
      for(final track in pattern) {
        track.remove('sound'); track.remove('notes'); track.remove('flags');
      }
    }
    final migrated = GrooveProject.decode(jsonEncode(old));
    expect(migrated.bpm,137); expect(migrated.selectedPattern,2);
    expect(migrated.patterns[0][0].mask,original.patterns[0][0].mask);
    expect(migrated.patterns[0][3].notes,GrooveTrack.defaultNotes);
    expect(jsonDecode(migrated.encode())['schemaVersion'],2);
  });
  test('sound, notes, slide and open-hat flags persist and are immutable', () {
    final notes=GrooveTrack.defaultNotes.toList(); notes[3]=51;
    final original=GrooveProject.initial().edit(1,3,
      GrooveTrack(15,4,.7,false,sound:const GrooveSound(waveform:1,cutoff:.7),notes:notes,flags:4));
    notes[3]=72;
    final restored=GrooveProject.decode(original.encode());
    expect(restored.encode(),original.encode());
    expect(restored.patterns[1][3].notes[3],51);
    expect(()=>restored.patterns[1][3].notes[3]=72,throwsUnsupportedError);
    for(final key in ['notes','flags','sound']) {
      final map=jsonDecode(original.encode()) as Map<String,dynamic>;
      map['patterns'][1][3].remove(key);
      expect(()=>GrooveProject.decode(jsonEncode(map)),throwsFormatException);
    }
    final map=jsonDecode(original.encode()) as Map<String,dynamic>;
    map['patterns'][1][3]['notes'][0]=99;
    expect(()=>GrooveProject.decode(jsonEncode(map)),throwsFormatException);
    map['patterns'][1][3]['notes'][0]=36;
    map['patterns'][1][3]['sound']['resonance']=1.1;
    expect(()=>GrooveProject.decode(jsonEncode(map)),throwsFormatException);
  });
  testWidgets('short Android window exposes controls without overflow', (tester) async {
    tester.view.physicalSize = const Size(780, 360);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.resetPhysicalSize);
    addTearDown(tester.view.resetDevicePixelRatio);
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      const MethodChannel('original_sequencer/project'), (call) async => null);
    await tester.pumpWidget(const PrototypeApp());
    await tester.pumpAndSettle();
    expect(find.text('Groovebox'), findsOneWidget);
    expect(find.text('キック'), findsOneWidget);
    expect(tester.takeException(), isNull);
    await tester.pumpWidget(const SizedBox.shrink());
  });
  testWidgets('export preserves unreadable original data instead of exporting defaults', (tester) async {
    const original='{"schemaVersion":99,"important":"keep me"}';
    String? exported;
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      const MethodChannel('original_sequencer/audio_focus'), (_) async => null);
    addTearDown(() => tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      const MethodChannel('original_sequencer/audio_focus'), null));
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      const MethodChannel('original_sequencer/project'),(call) async {
        if(call.method=='load') return original;
        if(call.method=='export') {exported=call.arguments as String;return true;}
        return null;
      });
    await tester.pumpWidget(const PrototypeApp()); await tester.pumpAndSettle();
    await tester.tap(find.byTooltip('プロジェクト')); await tester.pumpAndSettle();
    await tester.tap(find.text('ファイルへ書き出す')); await tester.pumpAndSettle();
    expect(exported,original);
    await tester.pumpWidget(const SizedBox.shrink());
  });
  testWidgets('unreadable save blocks overwriting', (tester) async {
    tester.binding.defaultBinaryMessenger.setMockMethodCallHandler(
      const MethodChannel('original_sequencer/project'), (call) async => '{"schemaVersion":99}');
    await tester.pumpWidget(const PrototypeApp());
    await tester.pumpAndSettle();
    expect(find.text('保存データを読めないため、上書き保存を停止しています'), findsOneWidget);
    final save = tester.widget<IconButton>(find.widgetWithIcon(IconButton, Icons.save_outlined));
    expect(save.onPressed, isNull);
    await tester.pumpWidget(const SizedBox.shrink());
  });
}
