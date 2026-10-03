import 'dart:ffi';
import 'dart:io';
import 'groove_project.dart';

// Field order and native types must match PrototypeDiagnostics in C.
final class _PrototypeDiagnosticsNative extends Struct {
  @Double()
  external double sampleRate;
  @Uint32()
  external int callbackFrames;
  @Uint32()
  external int callbackFramesMin;
  @Uint32()
  external int callbackFramesMax;
  @Uint64()
  external int renderedFrames;
  @Uint64()
  external int callbackStartFrame;
  @Double()
  external double callbackDurationUs;
  @Double()
  external double callbackLoad;
  @Double()
  external double callbackLoadP95;
  @Double()
  external double callbackLoadP99;
  @Double()
  external double callbackLoadPeak;
  @Uint32()
  external int audioRestartCount;
  @Uint32()
  external int queueDepth;
  @Uint32()
  external int queueHighWaterMark;
  @Uint64()
  external int queueOverflowCount;
  @Uint64()
  external int triggerCount;
  @Uint32()
  external int lastTriggerOffset;
}

typedef _CreateNative = Pointer<Void> Function();
typedef _CreateDart = Pointer<Void> Function();
typedef _DestroyNative = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);
typedef _StartAudioNative = Int32 Function(Pointer<Void>);
typedef _StartAudioDart = int Function(Pointer<Void>);
typedef _StopAudioNative = Void Function(Pointer<Void>);
typedef _StopAudioDart = void Function(Pointer<Void>);
typedef _GetDiagnosticsNative = _PrototypeDiagnosticsNative Function(Pointer<Void>);
typedef _GetDiagnosticsDart = _PrototypeDiagnosticsNative Function(Pointer<Void>);
typedef _ScheduleTriggerNative = Int32 Function(Pointer<Void>, Uint32, Float);
typedef _ScheduleTriggerDart = int Function(Pointer<Void>, int, double);

final class _SequenceNative extends Struct {
  @Double()
  external double bpm;
  @Uint32()
  external int running;
  @Uint32()
  external int stepMask;
  @Uint32()
  external int currentStep;
  @Uint64()
  external int missedSteps;
  @Uint32()
  external int currentPattern;
  @Uint32()
  external int queuedPattern;
}

typedef _PlayingNative = Int32 Function(Pointer<Void>, Int32);
typedef _PlayingDart = int Function(Pointer<Void>, int);
typedef _BpmNative = Int32 Function(Pointer<Void>, Double);
typedef _BpmDart = int Function(Pointer<Void>, double);
typedef _StepNative = Int32 Function(Pointer<Void>, Uint32, Int32);
typedef _StepDart = int Function(Pointer<Void>, int, int);
typedef _SequenceGetNative = _SequenceNative Function(Pointer<Void>);
typedef _SequenceGetDart = _SequenceNative Function(Pointer<Void>);

typedef _TrackNative = Int32 Function(Pointer<Void>, Uint32, Uint32, Uint32, Uint32, Float, Int32);
typedef _TrackDart = int Function(Pointer<Void>, int, int, int, int, double, int);
typedef _PatternNative = Int32 Function(Pointer<Void>, Uint32);
typedef _PatternDart = int Function(Pointer<Void>, int);

typedef _SoundNative = Int32 Function(Pointer<Void>, Uint32, Uint32, Float, Float, Float, Float, Float, Float, Uint32);
typedef _SoundDart = int Function(Pointer<Void>, int, int, double, double, double, double, double, double, int);
typedef _NoteNative = Int32 Function(Pointer<Void>, Uint32, Uint32, Uint32, Uint32, Int32);
typedef _NoteDart = int Function(Pointer<Void>, int, int, int, int, int);

final class _TrackConfigNative extends Struct {
  @Uint32() external int mask;
  @Uint32() external int accents;
  @Float() external double level;
  @Int32() external int muted;
  @Float() external double pitch;
  @Float() external double decay;
  @Float() external double tone;
  @Float() external double cutoff;
  @Float() external double resonance;
  @Float() external double envelope;
  @Uint32() external int waveform;
  @Uint32() external int flags;
  @Array(16) external Array<Uint32> notes;
}
typedef _UpdateNative = Int32 Function(Pointer<Void>, Uint32, Uint32, Pointer<_TrackConfigNative>);
typedef _UpdateDart = int Function(Pointer<Void>, int, int, Pointer<_TrackConfigNative>);
typedef _AllocNative = Pointer<Void> Function(IntPtr);
typedef _AllocDart = Pointer<Void> Function(int);
typedef _FreeNative = Void Function(Pointer<Void>);
typedef _FreeDart = void Function(Pointer<Void>);

class PrototypeSequenceState {
  const PrototypeSequenceState(this.bpm, this.running, this.stepMask,
    this.currentStep, this.missedSteps, [this.currentPattern = 0, this.queuedPattern = 4]);
  final double bpm;
  final bool running;
  final int stepMask;
  final int currentStep;
  final int missedSteps;
  final int currentPattern, queuedPattern;
}

class PrototypeDiagnostics {
  const PrototypeDiagnostics({
    required this.sampleRate,
    required this.callbackFrames,
    required this.callbackFramesMin,
    required this.callbackFramesMax,
    required this.renderedFrames,
    required this.callbackStartFrame,
    required this.callbackDurationUs,
    required this.callbackLoad,
    required this.callbackLoadP95,
    required this.callbackLoadP99,
    required this.callbackLoadPeak,
    required this.audioRestartCount,
    required this.queueDepth,
    required this.queueHighWaterMark,
    required this.queueOverflowCount,
    required this.triggerCount,
    required this.lastTriggerOffset,
  });

  final double sampleRate;
  final int callbackFrames;
  final int callbackFramesMin;
  final int callbackFramesMax;
  final int renderedFrames;
  final int callbackStartFrame;
  final double callbackDurationUs;
  final double callbackLoad;
  final double callbackLoadP95;
  final double callbackLoadP99;
  final double callbackLoadPeak;
  final int audioRestartCount;
  final int queueDepth;
  final int queueHighWaterMark;
  final int queueOverflowCount;
  final int triggerCount;
  final int lastTriggerOffset;
}

class PrototypeNativeBridge {
  PrototypeNativeBridge._(this._handle, this._destroy, this._startAudio,
    this._stopAudio, this._getDiagnostics, this._scheduleTrigger,
    this._setPlaying, this._setBpm, this._setStep, this._getSequence, this._setTrack, this._selectPattern, this._setSound, this._setNote, this._updateTrack, this._allocate, this._free);

  factory PrototypeNativeBridge.open() {
    if (!Platform.isAndroid) {
      throw UnsupportedError('The native audio bridge is currently wired for Android only.');
    }
    final library = DynamicLibrary.open('liboriginal_sequencer_flutter_bridge.so');
    final create = library.lookupFunction<_CreateNative, _CreateDart>('prototype_create');
    final destroy = library.lookupFunction<_DestroyNative, _DestroyDart>('prototype_destroy');
    final startAudio = library.lookupFunction<_StartAudioNative, _StartAudioDart>('prototype_start_audio');
    final stopAudio = library.lookupFunction<_StopAudioNative, _StopAudioDart>('prototype_stop_audio');
    final getDiagnostics = library.lookupFunction<_GetDiagnosticsNative, _GetDiagnosticsDart>('prototype_get_diagnostics');
    final scheduleTrigger = library.lookupFunction<_ScheduleTriggerNative, _ScheduleTriggerDart>('prototype_schedule_trigger');
    final setPlaying = library.lookupFunction<_PlayingNative, _PlayingDart>('prototype_set_playing');
    final setBpm = library.lookupFunction<_BpmNative, _BpmDart>('prototype_set_bpm');
    final setStep = library.lookupFunction<_StepNative, _StepDart>('prototype_set_step');
    final getSequence = library.lookupFunction<_SequenceGetNative, _SequenceGetDart>('prototype_get_sequence_state');
    final setTrack = library.lookupFunction<_TrackNative, _TrackDart>('prototype_set_track');
    final selectPattern = library.lookupFunction<_PatternNative, _PatternDart>('prototype_select_pattern');
    final setSound = library.lookupFunction<_SoundNative, _SoundDart>('prototype_set_sound');
    final setNote = library.lookupFunction<_NoteNative, _NoteDart>('prototype_set_note');
    final updateTrack = library.lookupFunction<_UpdateNative, _UpdateDart>('prototype_update_track');
    final libc = DynamicLibrary.process();
    final allocate = libc.lookupFunction<_AllocNative, _AllocDart>('malloc');
    final free = libc.lookupFunction<_FreeNative, _FreeDart>('free');
    final handle = create();
    if (handle == nullptr) throw StateError('prototype_create returned a null handle.');
    return PrototypeNativeBridge._(handle, destroy, startAudio,
      stopAudio, getDiagnostics, scheduleTrigger, setPlaying, setBpm, setStep, getSequence, setTrack, selectPattern, setSound, setNote, updateTrack, allocate, free);
  }

  final Pointer<Void> _handle;
  final _DestroyDart _destroy;
  final _StartAudioDart _startAudio;
  final _StopAudioDart _stopAudio;
  final _GetDiagnosticsDart _getDiagnostics;
  final _ScheduleTriggerDart _scheduleTrigger;
  final _PlayingDart _setPlaying;
  final _BpmDart _setBpm;
  final _StepDart _setStep;
  final _SequenceGetDart _getSequence;
  final _TrackDart _setTrack;
  final _PatternDart _selectPattern;
  final _SoundDart _setSound;
  final _NoteDart _setNote;
  final _UpdateDart _updateTrack;
  final _AllocDart _allocate;
  final _FreeDart _free;
  bool _disposed = false;
  bool updateTrack(int pattern, int track, GrooveTrack data) {
    _checkOpen();
    if(data.notes.length!=16) throw ArgumentError('Track requires 16 notes');
    final memory=_allocate(sizeOf<_TrackConfigNative>());
    if(memory==nullptr) throw StateError('Cannot allocate track state');
    try {
      final pointer=memory.cast<_TrackConfigNative>();
      final config=pointer.ref;
      config.mask=data.mask; config.accents=data.accents; config.level=data.level;
      config.muted=data.muted ? 1 : 0;
      config.pitch=data.sound.pitch; config.decay=data.sound.decay; config.tone=data.sound.tone;
      config.cutoff=data.sound.cutoff; config.resonance=data.sound.resonance; config.envelope=data.sound.envelope;
      config.waveform=data.sound.waveform; config.flags=data.flags;
      for(var i=0;i<16;i++) { config.notes[i]=data.notes[i]; }
      return _updateTrack(_handle,pattern,track,pointer)!=0;
    } finally { _free(memory); }
  }
  bool setSound(int pattern, int track, GrooveSound sound) {
    _checkOpen();
    return _setSound(_handle, pattern, track, sound.pitch, sound.decay, sound.tone,
      sound.cutoff, sound.resonance, sound.envelope, sound.waveform) != 0;
  }
  bool setNote(int pattern, int track, int step, int note, bool flag) {
    _checkOpen();
    return _setNote(_handle, pattern, track, step, note, flag ? 1 : 0) != 0;
  }

  bool setTrack(int pattern, int track, int mask, int accents, double level, bool muted) {
    _checkOpen();
    return _setTrack(_handle, pattern, track, mask, accents, level, muted ? 1 : 0) != 0;
  }
  bool selectPattern(int pattern) {
    _checkOpen();
    return _selectPattern(_handle, pattern) != 0;
  }

  bool setPlaying(bool playing) {
    _checkOpen();
    return _setPlaying(_handle, playing ? 1 : 0) != 0;
  }

  bool setBpm(double bpm) {
    _checkOpen();
    return _setBpm(_handle, bpm) != 0;
  }

  bool setStep(int step, bool enabled) {
    _checkOpen();
    if (step < 0 || step >= 16) throw RangeError.range(step, 0, 15);
    return _setStep(_handle, step, enabled ? 1 : 0) != 0;
  }

  PrototypeSequenceState sequenceState() {
    _checkOpen();
    final n = _getSequence(_handle);
    return PrototypeSequenceState(n.bpm, n.running != 0, n.stepMask,
      n.currentStep, n.missedSteps, n.currentPattern, n.queuedPattern);
  }

  void _checkOpen() {
    if (_disposed) throw StateError('PrototypeNativeBridge has already been disposed.');
  }

  bool startAudio() {
    _checkOpen();
    return _startAudio(_handle) != 0;
  }

  // Scheduled P3 test input; a separate low-latency Live Pad comes later.
  bool scheduleTrigger({int delayFrames = 0, double value = 1.0}) {
    _checkOpen();
    if (delayFrames < 0 || delayFrames > 96000 || !value.isFinite || value < 0 || value > 1) {
      throw ArgumentError('Invalid scheduled trigger parameters.');
    }
    return _scheduleTrigger(_handle, delayFrames, value) != 0;
  }

  void stopAudio() {
    if (!_disposed) _stopAudio(_handle);
  }

  PrototypeDiagnostics diagnostics() {
    _checkOpen();
    final n = _getDiagnostics(_handle);
    return PrototypeDiagnostics(
      sampleRate: n.sampleRate,
      callbackFrames: n.callbackFrames,
      callbackFramesMin: n.callbackFramesMin,
      callbackFramesMax: n.callbackFramesMax,
      renderedFrames: n.renderedFrames,
      callbackStartFrame: n.callbackStartFrame,
      callbackDurationUs: n.callbackDurationUs,
      callbackLoad: n.callbackLoad,
      callbackLoadP95: n.callbackLoadP95,
      callbackLoadP99: n.callbackLoadP99,
      callbackLoadPeak: n.callbackLoadPeak,
      audioRestartCount: n.audioRestartCount,
      queueDepth: n.queueDepth,
      queueHighWaterMark: n.queueHighWaterMark,
      queueOverflowCount: n.queueOverflowCount,
      triggerCount: n.triggerCount,
      lastTriggerOffset: n.lastTriggerOffset,
    );
  }

  void dispose() {
    if (_disposed) return;
    _stopAudio(_handle);
    _destroy(_handle);
    _disposed = true;
  }
}
