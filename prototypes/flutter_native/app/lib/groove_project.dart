import 'dart:convert';

class GrooveTrack {
  const GrooveTrack(this.mask, this.accents, this.level, this.muted);
  final int mask, accents;
  final double level;
  final bool muted;
  GrooveTrack copy({int? mask, int? accents, double? level, bool? muted}) =>
    GrooveTrack(mask ?? this.mask, accents ?? this.accents, level ?? this.level, muted ?? this.muted);
  Map<String, Object> toJson() => {'mask': mask, 'accents': accents, 'level': level, 'muted': muted};
}

class GrooveProject {
  GrooveProject(this.bpm, this.selectedPattern, List<List<GrooveTrack>> patterns)
    : patterns = List.unmodifiable(patterns.map((p) => List<GrooveTrack>.unmodifiable(p)));
  final double bpm;
  final int selectedPattern;
  final List<List<GrooveTrack>> patterns;
  static const names = ['キック', 'スネア', 'ハット', 'ベース'];
  static const engines = ['synth-kick', 'synth-snare', 'synth-hat', 'synth-bass'];
  factory GrooveProject.initial() => GrooveProject(120, 0, List.generate(4, (p) => [
    const GrooveTrack(0x1111, 0x0001, 0.8, false),
    const GrooveTrack(0x1010, 0x1010, 0.7, false),
    GrooveTrack(p == 0 ? 0x5555 : 0xffff, 0x1111, 0.35, false),
    GrooveTrack(p == 0 ? 0 : 0x0401, 1, 0.5, false),
  ]));
  GrooveProject copy({double? bpm, int? selectedPattern}) => GrooveProject(
    bpm ?? this.bpm, selectedPattern ?? this.selectedPattern, patterns);
  GrooveProject edit(int pattern, int track, GrooveTrack data) {
    final next = patterns.map((p) => p.toList()).toList();
    next[pattern][track] = data;
    return GrooveProject(bpm, selectedPattern, next);
  }
  String encode() => jsonEncode({'schemaVersion': 1, 'kind': 'android-groovebox', 'ppqn': 960,
    'bpm': bpm, 'selectedPattern': selectedPattern, 'engines': engines,
    'patterns': patterns.map((p) => p.map((t) => t.toJson()).toList()).toList()});
  factory GrooveProject.decode(String source) {
    final data = jsonDecode(source);
    if (data is! Map || data['schemaVersion'] != 1 || data['kind'] != 'android-groovebox' || data['ppqn'] != 960) {
      throw const FormatException('Unsupported project schema');
    }
    final bpm = data['bpm'];
    final selected = data['selectedPattern'];
    final patterns = data['patterns'];
    if (bpm is! num || !bpm.isFinite || bpm < 60 || bpm > 240 ||
        selected is! int || selected < 0 || selected > 3 ||
        patterns is! List || patterns.length != 4 || jsonEncode(data['engines']) != jsonEncode(engines)) {
      throw const FormatException('Invalid project structure');
    }
    final decoded = <List<GrooveTrack>>[];
    for (final pattern in patterns) {
      if (pattern is! List || pattern.length != 4) throw const FormatException('Invalid tracks');
      final tracks = <GrooveTrack>[];
      for (final t in pattern) {
        if (t is! Map) throw const FormatException('Invalid track');
        final mask = t['mask'], accents = t['accents'], level = t['level'], muted = t['muted'];
        if (mask is! int || mask < 0 || mask > 65535 || accents is! int || accents < 0 || accents > 65535 ||
            level is! num || !level.isFinite || level < 0 || level > 1 || muted is! bool) {
          throw const FormatException('Invalid track values');
        }
        tracks.add(GrooveTrack(mask, accents, level.toDouble(), muted));
      }
      decoded.add(tracks);
    }
    return GrooveProject(bpm.toDouble(), selected, decoded);
  }
}
