import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter/services.dart';
import 'groove_project.dart';
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
  GrooveProject _project = GrooveProject.initial();
  int _editingPattern = 0, _track = 0;
  Timer? _saveTimer;
  bool _saveBlocked = false;
  String _saveStatus = '読み込み中';
  final _audioStatus = ValueNotifier(const AudioStatus(null, null, null, null));

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addObserver(this);
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
    _openNativeBridge();
  }

  void _applyProject(PrototypeNativeBridge bridge) {
    if (!bridge.setBpm(_project.bpm)) throw StateError('BPM unavailable');
    for (var p = 0; p < 4; p++) {
      for (var t = 0; t < 4; t++) {
        final data = _project.patterns[p][t];
        if (!bridge.setTrack(p, t, data.mask, data.accents, data.level, data.muted)) {
          throw StateError('Track unavailable');
        }
      }
    }
    if (!bridge.selectPattern(_project.selectedPattern)) throw StateError('Pattern unavailable');
  }

  void _scheduleSave() {
    if (_saveBlocked) return;
    _saveTimer?.cancel();
    setState(() => _saveStatus = '未保存');
    _saveTimer = Timer(const Duration(milliseconds: 500), _saveProject);
  }

  Future<void> _saveProject() async {
    if (_saveBlocked) return;
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
    if (!bridge.setTrack(_editingPattern, _track, data.mask, data.accents, data.level, data.muted)) return;
    setState(() => _project = _project.edit(_editingPattern, _track, data));
    _scheduleSave();
  }

  void _openTrackControls() {
    showModalBottomSheet<void>(context: context, builder: (context) => StatefulBuilder(
      builder: (context, updateSheet) {
        final data = _project.patterns[_editingPattern][_track];
        return SafeArea(child: Padding(padding: const EdgeInsets.all(20), child: Column(
          mainAxisSize: MainAxisSize.min, children: [
            Text(GrooveProject.names[_track], style: Theme.of(context).textTheme.titleLarge),
            SwitchListTile(title: const Text('ミュート'), value: data.muted,
              onChanged: (value) { _editTrack(data.copy(muted: value)); updateSheet(() {}); }),
            Text('音量 ${(data.level * 100).round()}%'),
            Slider(value: data.level, onChanged: (value) {
              _editTrack(data.copy(level: value)); updateSheet(() {});
            }),
            const Text('ステップ長押しでアクセント（強い音）を切り替えます'),
          ],
        )));
      },
    ));
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
    if (state != AppLifecycleState.resumed && (_sequence?.running ?? false)) {
      _control((bridge) => bridge.setPlaying(false));
    }
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

  void _trigger() {
    final bridge = _bridge;
    if (bridge == null) return;
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
          onPressed: _saveBlocked ? null : () { _saveTimer?.cancel(); _saveProject(); }), IconButton(
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
              child: Text('${String.fromCharCode(65 + p)}${_sequence?.queuedPattern == p ? '…' : (_sequence?.currentPattern == p ? ' ▶' : '')}'),
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
            child: Text(_project.patterns[_editingPattern][_track].muted ? 'ミュート中 · 音量' : '音量・ミュート')),
        ]),
        Expanded(child: SequencerPanel(
          sequence: PrototypeSequenceState(_sequence?.bpm ?? _project.bpm, _sequence?.running ?? false,
            _project.patterns[_editingPattern][_track].mask,
            _sequence?.currentPattern == _editingPattern ? (_sequence?.currentStep ?? 16) : 16,
            _sequence?.missedSteps ?? 0), ready: ready,
          accentMask: _project.patterns[_editingPattern][_track].accents,
          onAccent: (step) {
            final data = _project.patterns[_editingPattern][_track];
            _editTrack(data.copy(accents: data.accents ^ (1 << step)));
          },
          onPlaying: (playing) => _control((b) => b.setPlaying(playing)),
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
