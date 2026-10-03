import 'dart:convert';

class GrooveSound {
  const GrooveSound({this.pitch = .5, this.decay = .35, this.tone = .5,
    this.cutoff = .35, this.resonance = .55, this.envelope = .5, this.waveform = 0});
  final double pitch, decay, tone, cutoff, resonance, envelope;
  final int waveform;
  GrooveSound copy({double? pitch, double? decay, double? tone, double? cutoff,
    double? resonance, double? envelope, int? waveform}) => GrooveSound(
      pitch: pitch ?? this.pitch, decay: decay ?? this.decay, tone: tone ?? this.tone,
      cutoff: cutoff ?? this.cutoff, resonance: resonance ?? this.resonance,
      envelope: envelope ?? this.envelope, waveform: waveform ?? this.waveform);
  Map<String, Object> toJson() => {'pitch': pitch, 'decay': decay, 'tone': tone,
    'cutoff': cutoff, 'resonance': resonance, 'envelope': envelope, 'waveform': waveform};
  factory GrooveSound.decode(dynamic data) {
    if (data is! Map) throw const FormatException('Missing sound');
    final values = <double>[];
    for (final key in ['pitch', 'decay', 'tone', 'cutoff', 'resonance', 'envelope']) {
      final value = data[key];
      if (value is! num || !value.isFinite || value < 0 || value > 1) {
        throw const FormatException('Invalid sound value');
      }
      values.add(value.toDouble());
    }
    final wave = data['waveform'];
    if (wave is! int || wave < 0 || wave > 1) throw const FormatException('Invalid waveform');
    return GrooveSound(pitch: values[0], decay: values[1], tone: values[2],
      cutoff: values[3], resonance: values[4], envelope: values[5], waveform: wave);
  }
}

class GrooveTrack {
  const GrooveTrack(this.mask, this.accents, this.level, this.muted,
    {this.sound = const GrooveSound(), this.notes = defaultNotes, this.flags = 0});
  static const defaultNotes = [36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36];
  final int mask, accents, flags;
  final double level;
  final bool muted;
  final GrooveSound sound;
  final List<int> notes;
  GrooveTrack copy({int? mask, int? accents, double? level, bool? muted,
    GrooveSound? sound, List<int>? notes, int? flags}) =>
    GrooveTrack(mask ?? this.mask, accents ?? this.accents, level ?? this.level, muted ?? this.muted,
      sound: sound ?? this.sound, notes: List.unmodifiable(notes ?? this.notes), flags: flags ?? this.flags);
  Map<String, Object> toJson() => {'mask': mask, 'accents': accents, 'level': level, 'muted': muted,
    'sound': sound.toJson(), 'notes': notes, 'flags': flags};
}

class GrooveProject {
  GrooveProject(this.bpm, this.selectedPattern, List<List<GrooveTrack>> patterns)
    : patterns = List.unmodifiable(patterns.map((p) => List<GrooveTrack>.unmodifiable(p.map((t) => t.copy()))));
  final double bpm;
  final int selectedPattern;
  final List<List<GrooveTrack>> patterns;
  static const names = ['キック', 'スネア', 'ハット', 'ベース'];
  static const engines = ['synth-kick', 'synth-snare', 'synth-hat', 'synth-bass'];
  factory GrooveProject.initial() => GrooveProject(120, 0, List.generate(4, (p) => [
    const GrooveTrack(0x1111, 0x0001, 0.8, false),
    const GrooveTrack(0x1010, 0x1010, 0.7, false),
    GrooveTrack(p == 0 ? 0x5555 : 0xffff, 0x1111, 0.35, false),
    GrooveTrack(0xdddd, 0x0101, 0.7, false, flags: 0x0404,
      notes: [36,36,36,36,43,43,48,48,36,36,39,39,43,43,34,34]),
  ]));
  GrooveProject copy({double? bpm, int? selectedPattern}) => GrooveProject(
    bpm ?? this.bpm, selectedPattern ?? this.selectedPattern, patterns);
  GrooveProject edit(int pattern, int track, GrooveTrack data) {
    final next = patterns.map((p) => p.toList()).toList();
    next[pattern][track] = data;
    return GrooveProject(bpm, selectedPattern, next);
  }
  String encode() => jsonEncode({'schemaVersion': 2, 'kind': 'android-groovebox', 'ppqn': 960,
    'bpm': bpm, 'selectedPattern': selectedPattern, 'engines': engines,
    'patterns': patterns.map((p) => p.map((t) => t.toJson()).toList()).toList()});
  factory GrooveProject.decode(String source) {
    final data = jsonDecode(source);
    if (data is! Map || data['schemaVersion'] is! int || ![1,2].contains(data['schemaVersion']) || data['kind'] != 'android-groovebox' || data['ppqn'] != 960) {
      throw const FormatException('Unsupported project schema');
    }
    final version = data['schemaVersion'];
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
        final sound = version == 1 ? const GrooveSound() : GrooveSound.decode(t['sound']);
        final notes = version == 1 ? GrooveTrack.defaultNotes : t['notes'];
        final flags = version == 1 ? 0 : t['flags'];
        if (notes is! List || notes.length != 16 || notes.any((n) => n is! int || n < 24 || n > 84) ||
            flags is! int || flags < 0 || flags > 65535) throw const FormatException('Invalid notes');
        tracks.add(GrooveTrack(mask, accents, level.toDouble(), muted, sound: sound,
          notes: List<int>.unmodifiable(notes.cast<int>()), flags: flags));
      }
      decoded.add(tracks);
    }
    return GrooveProject(bpm.toDouble(), selected, decoded);
  }
}
