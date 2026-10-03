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
      (m) => m['schemaVersion'] = 2,
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
