# Technology Prototype Scope

## 目的

Flutter UI + Native / C++ Audio LayerとJUCE / C++中心構成を、同じ製品要件・同じStress条件で比較します。

このPrototypeは製品v0.1を作るためのものではありません。

目的は、Phase 1へ進む前に以下を実機で判断できる証拠を得ることです。

- Realtime Audio要件を満たせるか
- Touch InstrumentとしてLatencyが許容できるか
- 32 Voice Baselineに十分なHeadroomがあるか
- Audio Device Lifecycleへ安全に対応できるか
- UI開発速度とMaintenance Costが許容できるか

合格条件の正本は`docs/platform-audio-requirements.md`、候補比較は`docs/framework-comparison.md`です。

## Prototype対象

### Candidate A

Flutter UI + Native / C++ Audio Layer

### Candidate B

JUCE / C++中心構成

Fully NativeはReference / Fallbackです。Web系は今回の比較対象に含めません。

## 共通Vertical Slice

両候補で次の同等機能を作ります。

### Screen

1画面だけで構いません。

表示:

- Play / Stop
- BPM
- 16-step Grid
- Sampler Trigger Row
- Live Trigger Pad
- Diagnostics Panel

製品版のVisual Design完成度は評価しません。

### Sequencer

- 4/4
- 1 Step = 1/16 note
- 16 Steps
- 960 PPQN相当のMusical Positionを保持可能な構造
- BPM変更
- Step On / Off
- Lookahead Scheduling
- Buffer内Sample OffsetでTrigger

Swing、Probability、Micro Timing等は不要です。

### Sound Source

最初は1つのNative Audio Engineを使います。

最低要件:

- One Shot Sample再生または同等の軽量Voice
- Overlap可能
- 複数Voice
- Voice Count計測

32 Voice Stressを安定して作るため、必要であれば軽量Oscillator Voiceを追加して構いません。

音質作り込みは目的ではありません。

### Live Input

Live Trigger PadはSequencer Lookahead Queue末尾へ入れません。

```text
Sequencer
  → Lookahead Scheduler
  → Scheduled Command Queue

Live Pad
  → Low-latency Command Path

両方
  → Audio Engine / Voice Manager
```

同じVoice Managerへ到達して構いません。

## Candidate Aの境界

Flutter側:

- UI
- Gesture
- BPM / Step編集Intent
- Diagnostics表示

Native / C++側:

- Audio Device Adapter
- SchedulerのRealtime側境界
- Bounded Queue
- Frame Timeline
- Voice Manager
- DSP / Sample Playback
- Callback Diagnostics

FlutterのUI Thread / Dart RuntimeをAudio Callbackから呼びません。

Audio CallbackごとのDart FFI往復もしません。

DiagnosticsはNative側で集計し、低頻度SnapshotとしてUIへ渡します。

初期候補:

- UI Snapshot更新: 10 Hz程度以下から開始
- Playhead表示はAudio Callbackから直接Pushせず、Frame Timelineを元にUI側で補間可能な構造を優先

## Candidate Bの境界

JUCE側:

- Touch UI
- Application Commands
- Scheduler
- Bounded Queue
- Voice Manager / DSP
- Audio Device Adapter
- Diagnostics

UI Event処理とAudio CallbackはThread境界を分けます。

C++だからといってUI StateをCallbackから直接触らないことを共通Ruleとします。

## Audio条件

優先試験条件:

- Sample Rate: 48 kHzを優先
- Preferred Buffer: 128 Frames程度
- Stable Fallback: 256 Frames程度
- 512 Frames以上でもLogicが破綻しないこと

Backendが要求値を採用しない場合、実際の値を記録します。

特定Buffer SizeをApplication LogicへHard-codeしません。

## Scheduling検証

最低限、次をTestします。

### A. Buffer内Trigger

同一Callback Buffer内の異なるOffsetへ複数Triggerを配置します。

期待:

- Buffer先頭へ丸められない
- Event順序を維持する
- OffsetがDiagnosticsで確認できる

### B. Boundary Burst

Step / Pattern Boundary相当で多数の同時Triggerを発生させます。

期待:

- Queue Overflowしない
- Callback Deadlineを継続的に超えない
- Voice Countが意図通り増える

### C. Live + Sequencer

Sequencer再生中にLive Padを連打します。

期待:

- Live Inputが50 ms Lookahead相当を待たない
- Sequencer Timingを壊さない
- UI Gesture負荷でDropoutしない

## Performance Scenarios

### Scenario 1: Baseline

- 48 kHz
- Preferred 128 Frames程度
- 32 Concurrent Light Voices
- Sequencer Trigger継続
- Diagnostics有効

目標:

- 安定再生
- Callback Load概ね50%以下をNormal Target
- 継続的70〜80%以上に張り付かない
- Queue Overflow 0
- Dropout / XRun 0を目標

### Scenario 2: Stretch

- 64 Light-to-Moderate Voices

これはMustではありません。

評価:

- 安定性
- Callback Peak / Percentile
- Voice Steal
- Buffer Fallbackの必要性

### Scenario 3: UI Stress

再生中に:

- Stepを連続Toggle
- Padを連打
- Screen Gestureを継続
- Diagnostics表示更新

期待:

UI負荷がAudio Callback Deadlineへ直接波及しないこと。

### Scenario 4: 10-minute Stress

Mixed / Burst負荷を約10分継続します。

記録:

- Dropout / XRun
- Callback Load Peak
- Callback Load Percentile
- Active Voice Peak
- Voice Steal Count
- Queue High-water Mark
- Queue Overflow Count

## Device Lifecycle試験

Mobileでは最低限以下を試します。

- App interruption
- Background / Foreground復帰
- Headphone接続 / 切断または可能なRoute Change
- Audio Device Restart

復帰時に確認:

1. 旧Callback停止
2. Pending Commands破棄
3. Voice停止 / 再初期化
4. 新Sample Rate / Buffer取得
5. Frame Origin再確立
6. Queue再構築
7. Transport Mapping再確立

旧Audio Frame Timelineをそのまま継続しません。

## Diagnostics最低要件

両候補で同名の概念を表示 / Logできるようにします。

```text
sampleRate
callbackFrames
callbackDurationUs
callbackLoad
callbackLoadPeak
callbackLoadPercentile
xrunOrDropoutCount
activeVoiceCount
peakVoiceCount
voiceStealCount
queueDepth
queueHighWaterMark
queueOverflowCount
audioRestartCount
```

Platformが直接XRunを提供しない場合は、取得可能な代替指標と測定方法を明記します。

## Live Input Latency測定

まずは2段階で評価します。

### 1. Instrumentation

記録:

- UI Touch Event Timestamp
- Native Command受付Timestamp
- Audio CallbackでVoice開始したFrame / Timestamp

これによりApplication内部の遅延を比較します。

### 2. End-to-end

可能であれば外部録音 / Loopback等を使い、Tapから実際のAudio Outputまでを測定します。

OSのTouch SamplingやHardware Output Latencyも含むため、Instrumentation値とは分けて記録します。

Bluetooth Routeは低遅延基準に使いません。

## Development Cost計測

性能だけでなく、同じ変更を両候補で行ったCostも記録します。

最低限記録:

- 初回Buildまでの障害
- 実機Deploy手順
- Native Debugger / Profilerの使いやすさ
- UI変更に必要なFile / Layer数
- Step Buttonを追加・変更する作業量
- Audio Parameterを1つUIへ公開する作業量
- Device Restart Bugを追跡する難易度

厳密な人時比較でなくても、差が分かる記録を残します。

## 非対象

今回作らないもの:

- Project Save / Load
- Sample Import UI
- Waveform Editor
- 完成版Sampler
- 完成版Synth
- Pattern管理UI
- Parameter Lock
- Swing
- Probability
- Ratchet
- Chord UI
- Effects
- MIDI
- Song Mode

## Prototype完了条件

候補ごとに次が揃ったら比較可能とします。

- 実機で16-step再生できる
- Buffer内Sample Offset Triggerを確認できる
- Live PadがLow-latency Pathで鳴る
- Actual Sample Rate / Buffer Framesを表示できる
- Callback Loadを計測できる
- 32 Voice Baselineを実行できる
- 10分Stress結果を保存できる
- Device Restart系Testを少なくとも1回完走できる
- UI Stress中のAudio Stabilityを確認できる
- Build / Debug / UI変更Costを記録できる

## 採用Gate

### Must Gate

以下のどれかに失敗する候補は原則として採用しません。

- Realtime Callbackへ十分にアクセスできない
- Buffer内Sample Offset Schedulingができない
- Audio CallbackがUI Runtime同期処理へ依存する
- Live Inputが明確に不自然な遅延になる
- Variable Bufferに対応できない
- 32 Voice Baselineが安定しない
- Device Restartから安全に復帰できない
- Diagnostics不足でRealtime問題を追跡できない

### Comparative Gate

両方がMustを通過した場合、次を比較します。

1. Callback Headroom
2. Live Input Latency
3. 10分Stress安定性
4. Mobile Lifecycleの実装複雑性
5. Touch UI Iteration速度
6. Debug / Profiling効率
7. Cross-platform Maintenance Cost
8. Desktop拡張性

## Prototype実装順序

各候補で同じ順序を推奨します。

1. Audio Device Callback + Sine Test
2. Variable Callback Frames / Diagnostics
3. Frame Timeline
4. Bounded Command Queue
5. Sample-accurate Trigger
6. Live Trigger Path
7. 16-step Scheduler
8. Lightweight Poly Voice
9. 32 Voice Baseline
10. UI Stress
11. Device Restart
12. 10-minute Stress
13. 64 Voice Stretch

先に完成UIを作らず、Realtime要件を小さく検証します。

## Prototype後のDecision

結果を`docs/decisions.md`へ記録し、選定したStackを`docs/status.md`と`docs/architecture.md`へ反映します。

採用理由だけでなく、不採用候補がどのGateで不利だったかも残します。
