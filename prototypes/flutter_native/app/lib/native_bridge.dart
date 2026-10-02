import 'dart:ffi';
import 'dart:io';

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
    this._stopAudio, this._getDiagnostics, this._scheduleTrigger);

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
    final handle = create();
    if (handle == nullptr) throw StateError('prototype_create returned a null handle.');
    return PrototypeNativeBridge._(handle, destroy, startAudio,
      stopAudio, getDiagnostics, scheduleTrigger);
  }

  final Pointer<Void> _handle;
  final _DestroyDart _destroy;
  final _StartAudioDart _startAudio;
  final _StopAudioDart _stopAudio;
  final _GetDiagnosticsDart _getDiagnostics;
  final _ScheduleTriggerDart _scheduleTrigger;
  bool _disposed = false;

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
