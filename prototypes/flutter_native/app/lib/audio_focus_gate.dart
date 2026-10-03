// Keeps an asynchronous Android focus reply from restarting playback after Stop,
// backgrounding or disposal. Platform I/O stays outside the sequencer model.
class AudioFocusGate {
  AudioFocusGate({required this.request, required this.abandon});
  final Future<bool> Function() request;
  final Future<void> Function() abandon;
  int _generation=0;
  bool _wanted=false, _disposed=false;
  Future<bool> acquire() async {
    if(_disposed) return false;
    _wanted=true;
    final generation=++_generation;
    bool granted;
    try { granted=await request(); } catch (_) { granted=false; }
    if(_disposed || !_wanted || generation!=_generation) {
      if(granted && !_wanted) await _release();
      return false;
    }
    return granted;
  }
  Future<void> _release() async { try { await abandon(); } catch (_) {} }
  Future<void> cancel() async { _wanted=false; ++_generation; await _release(); }
  void dispose() { _disposed=true; cancel(); }
}
