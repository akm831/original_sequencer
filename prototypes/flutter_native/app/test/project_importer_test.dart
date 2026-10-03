import 'package:flutter_test/flutter_test.dart';
import '../lib/project_importer.dart';
import '../lib/groove_project.dart';
void main() {
  test('invalid input is rejected before review, backup or commit',() async {
    var reviews=0,backups=0,commits=0;
    final importer=ProjectImporter(read:() async=>'{',confirm:(_) async {reviews++;return true;},
      backup:() async {backups++;return true;},commit:(_) async {commits++;});
    await expectLater(importer.run(),throwsFormatException);
    expect([reviews,backups,commits],[0,0,0]);
  });
  test('picker or review cancellation leaves the current project alone',() async {
    var commits=0,backups=0;
    for(final source in <String?>[null,GrooveProject.initial().encode()]) {
      final importer=ProjectImporter(read:() async=>source,confirm:(_) async=>false,
        backup:() async {backups++;return true;},commit:(_) async {commits++;});
      expect(await importer.run(),false);
    }
    expect([backups,commits],[0,0]);
  });
  test('failed backup prevents replacement',() async {
    var committed=false;
    final importer=ProjectImporter(read:() async=>GrooveProject.initial().encode(),
      confirm:(_) async=>true,backup:() async=>false,commit:(_) async {committed=true;});
    await expectLater(importer.run(),throwsStateError); expect(committed,false);
  });
  test('validated project commits only after review and successful backup',() async {
    final events=<String>[];
    final source=GrooveProject.initial().copy(bpm:167,selectedPattern:3).encode();
    final importer=ProjectImporter(read:() async=>source,
      confirm:(project) async { expect(project.bpm,167);events.add('review');return true; },
      backup:() async {events.add('backup');return true;},
      commit:(project) async {expect(project.encode(),source);events.add('commit');});
    expect(await importer.run(),true); expect(events,['review','backup','commit']);
  });
}
