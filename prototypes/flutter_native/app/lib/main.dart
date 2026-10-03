import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter/services.dart';
import 'groove_project.dart';
import 'audio_focus_gate.dart';
import 'native_bridge.dart';
import 'sequencer_panel.dart';
import 'diagnostics_screen.dart';

void main() => runApp(const PrototypeApp());

class PrototypeApp extends StatelessWidget {
  const PrototypeApp({super.key});
  @override
  Widget build(BuildContext context) => const MaterialApp(
    debugShowCheckedModeBanner: false,
    home: PrototypeScreen(),
  );
}

class PrototypeScreen extends StatefulWidget {
  const PrototypeScreen({super.key});
  @override
  State<PrototypeScreen> createState() => _PrototypeScreenState();
}

class _PrototypeScreenState extends State<PrototypeScreen>
    with SingleTickerProviderStateMixin, WidgetsBindingObserver {
  PrototypeNativeBridge? _bridge;
  PrototypeDiagnostics? _diagnostics;
  PrototypeSequenceState? _sequence;
  Timer? _diagnosticsTimer;
  late final Ticker _playheadTicker;
  Object? _bridgeError;
  String? _triggerStatus;
  static const _storage = MethodChannel('original_sequencer/project');
  static const _focusChannel = MethodChannel('original_sequencer/audio_focus');
  late final AudioFocusGate _focus;
  bool _foreground=true;
  int _playRequest=0;
  Timer? _testFocusTimer;
  GrooveProject _project = GrooveProject.initial();
  int _editingPattern = 0, _track = 0;
  Timer? _saveTimer;
  bool _saveBlocked = false, _loading = true;
  String _saveStatus = '読み込み中';
  final _audioStatus = ValueNotifier(const AudioStatus(null, null, null, null));

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
    _focus=AudioFocusGate(request:() async => await _focusChannel.invokeMethod<bool>('request') ?? false,
      abandon:() async { await _focusChannel.invokeMethod<void>('abandon'); });
    _focusChannel.setMethodCallHandler((call) async {
      if(call.method=='lost' && mounted) {
        await _setPlaying(false);
        if(mounted) ScaffoldMessenger.of(context).showSnackBar(const SnackBar(
          content:Text('音声の使用権が変わったため停止しました。再生ボタンで再開できます。')));
      }
    });
    _playheadTicker = createTicker((_) => _refreshPlayhead());
    _initialiseProject();
  }

  Future<void> _initialiseProject() async {
    try {
      final source = await _storage.invokeMethod<String>('load');
      if (!mounted) return;
      if (source != null) _project = GrooveProject.decode(source);
      _editingPattern = _project.selectedPattern;
      _saveStatus = source == null ? '新しいプロジェクト' : '保存内容を復元しました';
    } catch (error) {
      if (!mounted) return;
      _saveBlocked = true;
      _saveStatus = '保存データを読めないため、上書き保存を停止しています';
    }
    _loading = false;
    _openNativeBridge();
    if (mounted) setState(() {});
  }

  bool _sendTrack(PrototypeNativeBridge bridge, int p, int t, GrooveTrack data) =>
    bridge.updateTrack(p,t,data);

  void _applyProject(PrototypeNativeBridge bridge) {
    if (!bridge.setBpm(_project.bpm)) throw StateError('BPM unavailable');
    for (var p = 0; p < 4; p++) {
      for (var t = 0; t < 4; t++) {
        final data = _project.patterns[p][t];
        if (!_sendTrack(bridge,p,t,data)) {
          throw StateError('Track unavailable');
        }
      }
    }
    if (!bridge.selectPattern(_project.selectedPattern)) throw StateError('Pattern unavailable');
  }

  void _scheduleSave() {
    if (_saveBlocked || _loading) return;
    _saveTimer?.cancel();
    setState(() => _saveStatus = '未保存');
    _saveTimer = Timer(const Duration(milliseconds: 500), _saveProject);
  }

  Future<void> _saveProject() async {
    if (_saveBlocked || _loading) return;
    final source = _project.encode();
    try {
      await _storage.invokeMethod<bool>('save', source);
      if (mounted && source == _project.encode()) setState(() => _saveStatus = '保存済み');
    } catch (error) {
      if (mounted) setState(() => _saveStatus = '保存できませんでした。保存ボタンで再試行できます');
    }
  }

  void _editTrack(GrooveTrack data) {
    final bridge = _bridge;
    if (bridge == null) return;
    if (!_sendTrack(bridge,_editingPattern,_track,data)) return;
    setState(() => _project = _project.edit(_editingPattern, _track, data));
    _scheduleSave();
  }

  void _openTrackControls() {
    final pattern = _editingPattern, track = _track;
    showModalBottomSheet<void>(context: context, isScrollControlled: true,
      builder: (context) => StatefulBuilder(builder: (context, updateSheet) {
        final data = _project.patterns[pattern][track];
        void change(GrooveTrack next) { _editTrack(next); updateSheet(() {}); }
        Widget parameter(String label, double value, GrooveSound Function(double) apply) => Column(
          mainAxisSize: MainAxisSize.min, children: [
            Text('$label ${(value*100).round()}%'),
            Slider(value: value, onChanged: (v) => change(data.copy(sound: apply(v)))),
          ]);
        return SafeArea(child: SizedBox(height: MediaQuery.sizeOf(context).height*.8,
          child: SingleChildScrollView(child: Padding(padding: const EdgeInsets.all(20), child: Column(
            mainAxisSize: MainAxisSize.min, children: [
              Text('${GrooveProject.names[track]} · ${track==3 ? 'Acid Bass' : 'Analog Percussion'}',
                style: Theme.of(context).textTheme.titleLarge),
              SwitchListTile(title: const Text('ミュート'), value: data.muted,
                onChanged: (v) => change(data.copy(muted: v))),
              Text('音量 ${(data.level*100).round()}%'),
              Slider(value: data.level, onChanged: (v) => change(data.copy(level: v))),
              if (track==3) ...[
                SegmentedButton<int>(segments: const [
                  ButtonSegment(value: 0,label: Text('ノコギリ波')),
                  ButtonSegment(value: 1,label: Text('矩形波')),
                ], selected: {data.sound.waveform},
                  onSelectionChanged: (v) => change(data.copy(sound: data.sound.copy(waveform: v.first)))),
                parameter('カットオフ',data.sound.cutoff,(v)=>data.sound.copy(cutoff:v)),
                parameter('レゾナンス',data.sound.resonance,(v)=>data.sound.copy(resonance:v)),
                parameter('エンベロープ',data.sound.envelope,(v)=>data.sound.copy(envelope:v)),
              ] else ...[
                parameter('ピッチ',data.sound.pitch,(v)=>data.sound.copy(pitch:v)),
                parameter('トーン',data.sound.tone,(v)=>data.sound.copy(tone:v)),
              ],
              parameter('ディケイ',data.sound.decay,(v)=>data.sound.copy(decay:v)),
              const Text('ステップ長押しで詳細編集。ベースは音程とスライド、ハットは開閉を設定できます。'),
            ],
          )))));
      }));
  }

  void _openStepEditor(int step) {
    final pattern = _editingPattern, track = _track;
    showModalBottomSheet<void>(context: context, isScrollControlled: true,
      builder: (context) => StatefulBuilder(builder: (context, updateSheet) {
        final data = _project.patterns[pattern][track];
        void change(GrooveTrack next) { _editTrack(next); updateSheet(() {}); }
        Widget toggle(String label, bool value, ValueChanged<bool> onChanged) =>
          SwitchListTile(title: Text(label),value: value,onChanged: onChanged);
        return SafeArea(child: SingleChildScrollView(child: Padding(padding: const EdgeInsets.all(20),child: Column(
          mainAxisSize: MainAxisSize.min, children: [
            Text('${GrooveProject.names[track]} · ステップ ${step+1}',style: Theme.of(context).textTheme.titleLarge),
            toggle('発音', (data.mask & (1<<step)) != 0,
              (v)=>change(data.copy(mask: v ? data.mask | (1<<step) : data.mask & ~(1<<step)))),
            toggle('アクセント', (data.accents & (1<<step)) != 0,
              (v)=>change(data.copy(accents: data.accents ^ (1<<step)))),
            if(track==3) ...[
              Text('音程 ${noteName(data.notes[step])}'),
              Slider(min:24,max:84,divisions:60,value:data.notes[step].toDouble(),
                label:noteName(data.notes[step]),onChanged:(v) {
                  final notes=data.notes.toList(); notes[step]=v.round(); change(data.copy(notes:notes));
                }),
              toggle('次のステップへスライド', (data.flags & (1<<step)) != 0,
                (v)=>change(data.copy(flags:data.flags ^ (1<<step)))),
              const Text('直後のステップが発音する場合に、音をつないで音程を滑らかに変えます。'),
            ],
            if(track==2) toggle('オープンハット', (data.flags & (1<<step)) != 0,
              (v)=>change(data.copy(flags:data.flags ^ (1<<step)))),
          ],
        ))));
      }));
  }

  void _openNativeBridge() {
    PrototypeNativeBridge? bridge;
    try {
      bridge = PrototypeNativeBridge.open();
      if (!bridge.startAudio()) {
        throw StateError('Native audio stream failed to start.');
      }
      _applyProject(bridge);
      _bridge = bridge;
      _refreshDiagnostics();
      _diagnosticsTimer = Timer.periodic(
        const Duration(milliseconds: 200),
        (_) => _refreshDiagnostics(),
      );
    } catch (error) {
      bridge?.dispose();
      _bridge = null;
      _bridgeError = error;
      _publishStatus();
    }
  }

  void _refreshDiagnostics() {
    final bridge = _bridge;
    if (bridge == null) return;
    try {
      final diagnostics = bridge.diagnostics();
      final sequence = bridge.sequenceState();
      if ((_sequence?.running ?? false) && !sequence.running) _focus.cancel();
      if (mounted) {
        setState(() {
          _diagnostics = diagnostics;
          _sequence = sequence;
        });
        _syncPlayheadTicker();
        _publishStatus();
      }
    } catch (error) {
      if (mounted) setState(() => _bridgeError = error);
      _publishStatus();
    }
  }

  // The native audio playhead is authoritative. Observe it before each paint;
  // this ticker never schedules notes or extrapolates the musical clock.
  void _refreshPlayhead() {
    final bridge = _bridge;
    if (bridge == null || !mounted) return;
    try {
      final next = bridge.sequenceState();
      final previous = _sequence;
      if ((previous?.running ?? false) && !next.running) _focus.cancel();
      if (previous == null ||
          next.currentStep != previous.currentStep ||
          next.running != previous.running ||
          next.bpm != previous.bpm ||
          next.stepMask != previous.stepMask ||
          next.missedSteps != previous.missedSteps ||
          next.currentPattern != previous.currentPattern || next.queuedPattern != previous.queuedPattern) {
        setState(() => _sequence = next);
      }
      _syncPlayheadTicker();
    } catch (error) {
      _playheadTicker.stop();
      setState(() => _bridgeError = error);
      _publishStatus();
    }
  }

  void _syncPlayheadTicker() {
    if (_sequence?.running ?? false) {
      if (!_playheadTicker.isActive) _playheadTicker.start();
    } else if (_playheadTicker.isActive) {
      _playheadTicker.stop();
    }
  }

  void _publishStatus() {
    _audioStatus.value = AudioStatus(_diagnostics, _sequence, _bridgeError, _triggerStatus);
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    // Leaving the app always stops transport; returning never auto-plays.
    if (state != AppLifecycleState.resumed) { _saveTimer?.cancel(); _saveProject(); }
    _foreground=state==AppLifecycleState.resumed;
    if (!_foreground) _setPlaying(false);
  }

  Future<void> _setPlaying(bool playing) async {
    final request=++_playRequest;
    _testFocusTimer?.cancel();
    if(!playing) {
      _control((b)=>b.setPlaying(false));
      await _focus.cancel();
      return;
    }
    if(!_foreground) return;
    final granted=await _focus.acquire();
    if(!mounted || !_foreground || request!=_playRequest) return;
    if(!granted) {
      ScaffoldMessenger.of(context).showSnackBar(const SnackBar(
        content:Text('音声を使用できません。ほかのアプリの再生を止めて再試行してください。')));
      return;
    }
    final accepted=_control((b) {
      if(!b.selectPattern(_editingPattern)) return false;
      return b.setPlaying(true);
    });
    if(!accepted) await _focus.cancel();
  }

  bool _control(bool Function(PrototypeNativeBridge) action) {
    final bridge = _bridge;
    if (bridge == null) return false;
    try {
      if (!action(bridge)) throw StateError('Audio control unavailable.');
      _refreshDiagnostics();
      return true;
    } catch (error) {
      setState(() => _bridgeError = error);
      _publishStatus();
      return false;
    }
  }

  Future<void> _trigger() async {
    final bridge = _bridge;
    if (bridge == null || !_foreground) return;
    final request=++_playRequest;
    final granted=await _focus.acquire();
    if(!mounted || !_foreground || !granted || request!=_playRequest) return;
    _testFocusTimer?.cancel();
    _testFocusTimer=Timer(const Duration(milliseconds:100),() {
      if(!(_sequence?.running ?? false)) _focus.cancel();
    });
    try {
      final accepted = bridge.scheduleTrigger(delayFrames: 37);
      setState(() {
        _triggerStatus = accepted
          ? 'テスト音を受け付けました'
          : 'テスト音を受け付けられませんでした';
      });
      _refreshDiagnostics();
    } catch (error) {
      setState(() => _bridgeError = error);
      _publishStatus();
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    ++_playRequest;
    _testFocusTimer?.cancel();
    _focusChannel.setMethodCallHandler(null);
    _focus.dispose();
    _saveTimer?.cancel();
    _playheadTicker.dispose();
    _audioStatus.dispose();
    _diagnosticsTimer?.cancel();
    _bridge?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final d = _diagnostics;
    final ready = _bridge != null && d != null && d.sampleRate > 0;
    return Scaffold(
      appBar: AppBar(
        title: const Text('Groovebox'),
        actions: [IconButton(tooltip: _saveStatus, icon: const Icon(Icons.save_outlined),
          onPressed: _saveBlocked || _loading ? null : () { _saveTimer?.cancel(); _saveProject(); }), IconButton(
          tooltip: '音声診断', icon: const Icon(Icons.monitor_heart_outlined),
          onPressed: () => Navigator.of(context).push(MaterialPageRoute<void>(
            builder: (_) => DiagnosticsScreen(status: _audioStatus, onTest: _trigger),
          )),
        )],
      ),
      body: LayoutBuilder(builder: (context, constraints) {
        final content = Column(children: [
        Padding(padding: const EdgeInsets.symmetric(horizontal: 12), child: Row(
          children: List.generate(4, (p) => Expanded(child: Padding(
            padding: const EdgeInsets.all(3), child: OutlinedButton(
              onPressed: ready ? () {
                if (!_control((b) => b.selectPattern(p))) return;
                setState(() { _editingPattern = p; _project = _project.copy(selectedPattern: p); });
                _scheduleSave();
              } : null,
              style: OutlinedButton.styleFrom(backgroundColor: _editingPattern == p ? Colors.deepPurple.shade50 : null),
              child: Text('${String.fromCharCode(65 + p)}${_sequence?.queuedPattern == p ? '…' : ((_sequence?.running ?? false) && _sequence?.currentPattern == p ? ' ▶' : '')}'),
            ),
          ))),
        )),
        Padding(padding: const EdgeInsets.symmetric(horizontal: 12), child: Row(
          children: List.generate(4, (t) => Expanded(child: TextButton(
            onPressed: () => setState(() => _track = t),
            style: TextButton.styleFrom(backgroundColor: _track == t ? Colors.deepPurple.shade50 : null),
            child: Text(GrooveProject.names[t], style: const TextStyle(fontSize: 12)),
          ))),
        )),
        Row(children: [
          const SizedBox(width: 16),
          Expanded(child: Text(_saveStatus, maxLines: 1, overflow: TextOverflow.ellipsis, style: const TextStyle(fontSize: 12))),
          TextButton(onPressed: ready ? _openTrackControls : null,
            child: Text(_project.patterns[_editingPattern][_track].muted ? 'ミュート中 · 音色' : '音色・音量')),
        ]),
        Expanded(child: SequencerPanel(
          sequence: PrototypeSequenceState(_sequence?.bpm ?? _project.bpm, _sequence?.running ?? false,
            _project.patterns[_editingPattern][_track].mask,
            _sequence?.currentPattern == _editingPattern ? (_sequence?.currentStep ?? 16) : 16,
            _sequence?.missedSteps ?? 0), ready: ready,
          accentMask: _project.patterns[_editingPattern][_track].accents,
          onAccent: _openStepEditor,
          notes: _track==3 ? _project.patterns[_editingPattern][_track].notes : null,
          flags: _project.patterns[_editingPattern][_track].flags,
          bass: _track==3, hat: _track==2,
          onPlaying: _setPlaying,
          onBpm: (bpm) {
            if (!_control((b) => b.setBpm(bpm))) return;
            setState(() => _project = _project.copy(bpm: bpm)); _scheduleSave();
          },
          onStep: (step, enabled) {
            final data = _project.patterns[_editingPattern][_track];
            _editTrack(data.copy(mask: enabled ? data.mask | (1 << step) : data.mask & ~(1 << step)));
          },
        )),
      ]);
        if (constraints.maxHeight < 500) {
          return SingleChildScrollView(child: SizedBox(height: 650, child: content));
        }
        return content;
      }),
    );
  }
}
