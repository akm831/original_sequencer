import 'dart:ffi';
import 'dart:io';

final class _PrototypeDiagnosticsNative extends Struct {
  @Double()
  external double sampleRate;

  @Uint32()
  external int callbackFrames;

  @Uint64()
  external int renderedFrames;

  @Uint32()
  external int audioRestartCount;
}

typedef _CreateNative = Pointer<Void> Function();
typedef _CreateDart = Pointer<Void> Function();
typedef _DestroyNative = Void Function(Pointer<Void>);
typedef _DestroyDart = void Function(Pointer<Void>);
typedef _GetDiagnosticsNative = _PrototypeDiagnosticsNative Function(Pointer<Void>);
typedef _GetDiagnosticsDart = _PrototypeDiagnosticsNative Function(Pointer<Void>);

class PrototypeDiagnostics {
  const PrototypeDiagnostics({
    required this.sampleRate,
    required this.callbackFrames,
    required this.renderedFrames,
    required this.audioRestartCount,
  });

  final double sampleRate;
  final int callbackFrames;
  final int renderedFrames;
  final int audioRestartCount;
}

class PrototypeNativeBridge {
  PrototypeNativeBridge._(this._library, this._handle, this._destroy, this._getDiagnostics);

  factory PrototypeNativeBridge.open() {
    if (!Platform.isAndroid) {
      throw UnsupportedError('The P0 Flutter native bridge is currently wired for Android only.');
    }

    final library = DynamicLibrary.open('liboriginal_sequencer_flutter_bridge.so');
    final create = library.lookupFunction<_CreateNative, _CreateDart>('prototype_create');
    final destroy = library.lookupFunction<_DestroyNative, _DestroyDart>('prototype_destroy');
    final getDiagnostics = library.lookupFunction<_GetDiagnosticsNative, _GetDiagnosticsDart>(
      'prototype_get_diagnostics',
    );
    final handle = create();

    if (handle == nullptr) {
      throw StateError('prototype_create returned a null handle.');
    }

    return PrototypeNativeBridge._(library, handle, destroy, getDiagnostics);
  }

  final DynamicLibrary _library;
  final Pointer<Void> _handle;
  final _DestroyDart _destroy;
  final _GetDiagnosticsDart _getDiagnostics;
  bool _disposed = false;

  PrototypeDiagnostics diagnostics() {
    if (_disposed) {
      throw StateError('PrototypeNativeBridge has already been disposed.');
    }

    final native = _getDiagnostics(_handle);
    return PrototypeDiagnostics(
      sampleRate: native.sampleRate,
      callbackFrames: native.callbackFrames,
      renderedFrames: native.renderedFrames,
      audioRestartCount: native.audioRestartCount,
    );
  }

  void dispose() {
    if (_disposed) {
      return;
    }
    _destroy(_handle);
    _disposed = true;
  }
}
