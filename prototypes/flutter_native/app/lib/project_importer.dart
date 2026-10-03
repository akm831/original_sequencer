import 'groove_project.dart';

// Parse and review before any replacement. A failed backup stops the import.
class ProjectImporter {
  ProjectImporter({required this.read, required this.confirm, required this.backup, required this.commit});
  final Future<String?> Function() read;
  final Future<bool> Function(GrooveProject) confirm;
  final Future<bool> Function() backup;
  final Future<void> Function(GrooveProject) commit;
  Future<bool> run() async {
    final source=await read();
    if(source==null) return false;
    final project=GrooveProject.decode(source);
    if(!await confirm(project)) return false;
    if(!await backup()) throw StateError('Cannot back up the existing project');
    await commit(project);
    return true;
  }
}
