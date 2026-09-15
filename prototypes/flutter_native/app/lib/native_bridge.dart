import 'dart:ffi';
import 'dart:io';
final class _PrototypeDiagnosticsNative extends Struct {
  @Double() external double sampleRate;
  @Uint32() external int callbackFrames;
  @Uint32() external int callbackFramesMin;
  @Uint32() external int callbackFramesMax;
  @Uint64() external int renderedFrames;
  @Uint64() external int callbackStartFrame;
  @Double() external double callbackDurationUs;
  @Double() external double callbackLoad;
  @Double() external double callbackLoadP95;
  @Double() external double callbackLoadP99;
  @Double() external double callbackLoadPeak;
  @Uint32() external int audioRestartCount;
}
typedef _CreateNative = Pointer<Void> Function(); typedef _CreateDart = Pointer<Void> Function();
typedef _DestroyNative = Void Function(Pointer<Void>); typedef _DestroyDart = void Function(Pointer<Void>);
typedef _StartAudioNative = Int32 Function(Pointer<Void>); typedef _StartAudioDart = int Function(Pointer<Void>);
typedef _StopAudioNative = Void Function(Pointer<Void>); typedef _StopAudioDart = void Function(Pointer<Void>);
typedef _GetDiagnosticsNative = _PrototypeDiagnosticsNative Function(Pointer<Void>); typedef _GetDiagnosticsDart = _PrototypeDiagnosticsNative Function(Pointer<Void>);
class PrototypeDiagnostics {
  const PrototypeDiagnostics({required this.sampleRate, required this.callbackFrames, required this.callbackFramesMin, required this.callbackFramesMax, required this.renderedFrames, required this.callbackStartFrame, required this.callbackDurationUs, required this.callbackLoad, required this.callbackLoadP95, required this.callbackLoadP99, required this.callbackLoadPeak, required this.audioRestartCount});
  final double sampleRate; final int callbackFrames; final int callbackFramesMin; final int callbackFramesMax; final int renderedFrames; final int callbackStartFrame; final double callbackDurationUs; final double callbackLoad; final double callbackLoadP95; final double callbackLoadP99; final double callbackLoadPeak; final int audioRestartCount;
}
class PrototypeNativeBridge {
  PrototypeNativeBridge._(this._library, this._handle, this._destroy, this._startAudio, this._stopAudio, this._getDiagnostics);
  factory PrototypeNativeBridge.open() {
    if (!Platform.isAndroid) throw UnsupportedError('The Flutter native audio bridge is currently wired for Android only.');
    final library = DynamicLibrary.open('liboriginal_sequencer_flutter_bridge.so');
    final create = library.lookupFunction<_CreateNative, _CreateDart>('prototype_create');
    final destroy = library.lookupFunction<_DestroyNative, _DestroyDart>('prototype_destroy');
    final startAudio = library.lookupFunction<_StartAudioNative, _StartAudioDart>('prototype_start_audio');
    final stopAudio = library.lookupFunction<_StopAudioNative, _StopAudioDart>('prototype_stop_audio');
    final getDiagnostics = library.lookupFunction<_GetDiagnosticsNative, _GetDiagnosticsDart>('prototype_get_diagnostics');
    final handle = create(); if (handle == nullptr) throw StateError('prototype_create returned a null handle.');
    return PrototypeNativeBridge._(library, handle, destroy, startAudio, stopAudio, getDiagnostics);
  }
  final DynamicLibrary _library; final Pointer<Void> _handle; final _DestroyDart _destroy; final _StartAudioDart _startAudio; final _StopAudioDart _stopAudio; final _GetDiagnosticsDart _getDiagnostics; bool _disposed = false;
  bool startAudio() { if (_disposed) throw StateError('PrototypeNativeBridge has already been disposed.'); return _startAudio(_handle) != 0; }
  void stopAudio() { if (!_disposed) _stopAudio(_handle); }
  PrototypeDiagnostics diagnostics() { if (_disposed) throw StateError('PrototypeNativeBridge has already been disposed.'); final n = _getDiagnostics(_handle); return PrototypeDiagnostics(sampleRate:n.sampleRate, callbackFrames:n.callbackFrames, callbackFramesMin:n.callbackFramesMin, callbackFramesMax:n.callbackFramesMax, renderedFrames:n.renderedFrames, callbackStartFrame:n.callbackStartFrame, callbackDurationUs:n.callbackDurationUs, callbackLoad:n.callbackLoad, callbackLoadP95:n.callbackLoadP95, callbackLoadP99:n.callbackLoadP99, callbackLoadPeak:n.callbackLoadPeak, audioRestartCount:n.audioRestartCount); }
  void dispose() { if (_disposed) return; _stopAudio(_handle); _destroy(_handle); _disposed = true; }
}
