# Technology Prototype実装計画

## 目的

`docs/technology-prototype.md`で確定した比較Scopeを、Flutter UI + Native / C++ AudioとJUCE / C++の2候補で公平に実装できるRepository構造とCheckpointへ落とします。

この文書はPrototype実装のための計画です。製品版Architectureの最終決定ではありません。

## 基本方針

- 2候補を同じRepository内で管理する
- DSP Algorithm差をFramework差として誤認しないよう、Framework非依存の小さなReference Audio Coreを共有する
- UI AdapterとPlatform Audio Backendは候補ごとに分ける
- Benchmark Scenarioと結果Formatは共有する
- 片方だけ先に大きく作り込まず、同じCheckpoint単位で比較する
- Prototype用コードをそのまま製品版へ昇格させることを前提にしない

## 推奨Directory構造

```text
original_sequencer/
├─ docs/
│  ├─ technology-prototype.md
│  ├─ prototype-implementation-plan.md
│  └─ prototype-results/
│     ├─ README.md
│     ├─ flutter-native/
│     └─ juce/
│
├─ prototypes/
│  ├─ common/
│  │  ├─ audio_core/
│  │  ├─ benchmark/
│  │  └─ test_assets/
│  │
│  ├─ flutter_native/
│  │  ├─ app/
│  │  ├─ native/
│  │  └─ platform/
│  │
│  └─ juce/
│     ├─ app/
│     └─ platform/
│
└─ ...
```

実際のBuild Tool都合で内部Directory名は調整して構いませんが、責務境界は維持します。

## `prototypes/common/audio_core`

Framework非依存のC++ Coreです。

ここへ置くもの:

- Audio frame timeline
- Realtime command表現
- Bounded queue
- Minimal scheduler側契約
- Voice lifecycle
- Minimal sampler voice
- Minimal oscillator voice
- Mixing
- Callback diagnostics集計
- Benchmark用synthetic voice生成

ここへ置かないもの:

- Flutter / Dart型
- JUCE UI型
- Android View / Kotlin型
- iOS UIKit / SwiftUI型
- JUCE Component型
- Platform audio session API
- File picker
- Production persistence

### Dependency Rule

```text
Candidate Adapter / Backend
          ↓
Reference Audio Core
```

Reference Audio CoreからFlutter/JUCE/Platform APIを参照しません。

## `prototypes/common/benchmark`

候補間で同じStress条件を再現する定義を置きます。

最低限:

- 32 Voice Baseline Scenario
- 64 Voice Stretch Scenario
- Boundary Burst Scenario
- Live + Sequencer Scenario
- 10-minute Stress Scenario
- Expected trigger timestamps / frame offsets

BenchmarkはUI操作だけに依存させず、同じCommand列をNative側へ投入できるHeadlessまたはDebug経路を持たせます。

これによりTouch操作速度の差とAudio Engine性能差を分けて確認できます。

## `prototypes/common/test_assets`

比較に使う小さな再配布可能Assetだけを置きます。

条件:

- Licenseが明確
- 短いPCM素材
- 候補A/Bで完全に同じFile
- Decode差を避けるため、最初は単純なWAV等を優先

Prototype比較に不要なFactory Libraryは追加しません。

## Candidate A: `prototypes/flutter_native`

### `app`

Flutter / Dart UI。

担当:

- 1 Screen
- Play / Stop
- BPM
- 16-step Grid
- Live Pad
- Diagnostics表示
- UI state / interaction

Realtime Audio処理は置きません。

### `native`

C/C++ BridgeとApplication Adapter。

担当:

- Dart FFI用の薄いC ABI
- UI Command → Core Command変換
- Diagnostics Snapshot取得
- Core lifecycle

### `platform`

Platform Audio Adapter。

担当候補:

- Android: Oboe / AAudio系
- iOS: Native low-latency audio API + AVAudioSession lifecycle

CallbackではReference Audio Coreのrender入口を呼びます。

## Candidate B: `prototypes/juce`

### `app`

JUCE UIとApplication Adapter。

担当:

- 1 Screen
- Play / Stop
- BPM
- 16-step Grid
- Live Pad
- Diagnostics表示
- UI Command → Core Command変換

### `platform`

JUCE Audio Device / Platform lifecycle Adapter。

JUCE Audio CallbackからReference Audio Coreのrender入口を呼びます。

Platform固有のAudio Session / Route Change処理が必要な場合はここへ閉じ込めます。

## Reference Audio Coreの最小契約

最初から完成版Engine APIを設計しません。

Prototypeで必要な契約だけを持ちます。

概念上は次の4境界で十分です。

### 1. Lifecycle

```text
initialize(actualSampleRate, maxCallbackFrames)
reset()
shutdown()
```

目的:

- Device開始
- Restart
- Sample Rate / Buffer変更

を扱うこと。

### 2. Command Input

Realtime-safeな固定サイズCommandをQueueへ投入します。

概念例:

```text
Command
- type
- targetFrame
- trackId
- noteOrVoiceData
- value
```

最初に必要なCommand:

- TriggerSampler
- TriggerOscillator
- NoteOff / StopVoice相当
- TransportStart
- TransportStop
- Panic

UI専用Objectや可変長StringをCommandへ入れません。

### 3. Render

Platform callbackから呼ぶ最小境界:

```text
render(outputBuffer, frameCount, callbackStartFrame)
```

Core内部で:

1. 対象Buffer範囲のCommandを取得
2. targetFrameからBuffer Offsetを計算
3. Offset位置でTrigger
4. VoiceをRender
5. Mix
6. Diagnostics更新

を行います。

### 4. Diagnostics Snapshot

UI Thread側から低頻度で取得します。

最低限:

```text
sampleRate
callbackFrames
callbackLoadAverage
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

Snapshot取得のためにAudio CallbackをBlockしません。

## C++ CoreとFlutter FFIの境界

FlutterからC++ classを直接意識させません。

Native層で薄いC ABIを公開します。

概念例:

```text
prototype_create()
prototype_destroy()
prototype_start()
prototype_stop()
prototype_set_bpm()
prototype_set_step()
prototype_live_trigger()
prototype_get_diagnostics()
```

これは製品版APIの確定形ではありません。

目的は:

- Dart/C++境界を小さくする
- Callback内FFIを避ける
- Candidate AのBridge Costを観測しやすくする

ことです。

## JUCE境界

JUCE側もUIからAudio Core内部を直接操作しません。

```text
JUCE Component
    ↓
Application Command Adapter
    ↓
Reference Audio Core Queue
```

C++で同一Processだからといって、UI ObjectへのPointerをAudio Threadから触る実装は禁止します。

## Sequencer / SchedulerのPrototype Scope

製品版Sequencer Model全体は作りません。

Prototypeでは以下だけを実装します。

- BPM
- running state
- 16 bool steps / track
- 2 track相当
- 1 Step = 1/16 note
- 960 PPQNへ対応可能なMusical Position
- LookaheadでtargetFrame Commandを生成

Pattern、Persistence、Advanced Step Dataはまだ不要です。

### Lookahead

初期候補は約50 ms。

ただし固定値をAudio Callbackへ埋め込みません。

Live PadはLookahead Schedulerを経由せず、Low-latency Command Pathから次の安全なAudio処理機会へ届けます。

## Queue方針

PrototypeでもQueue安全性は比較対象なので、仮のunbounded containerへ逃げません。

最低条件:

- Bounded
- Audio ThreadでBlockしない
- Queue Overflow Countを取る
- High-water Markを取る
- Commandは固定サイズを基本とする

SPSCで成立するCommand経路から開始します。

複数Producerが必要になった場合は、Audio Thread側を複雑にする前にProducer統合層を検討します。

## Diagnostics計測位置

Callback LoadはPlatform callback入口〜Audio処理完了までを共通定義とします。

概念:

```text
callbackLoad
= callbackProcessingTime / callbackBufferDuration
```

Candidate固有のUI描画時間はCallback Loadへ含めません。

ただしUI Stress ScenarioではAudio LoadとUI Frame挙動を同時に記録します。

## Result保存Format

`docs/prototype-results/`にHuman-readable Markdownを残します。

1 runにつき最低限:

```text
candidate:
commit:
platform:
device:
osVersion:
audioRoute:
sampleRate:
callbackFrames:
scenario:
duration:
callbackLoadAverage:
callbackLoadPeak:
callbackLoadP95:
dropoutCount:
peakVoiceCount:
voiceStealCount:
queueHighWaterMark:
queueOverflowCount:
liveInputLatencyInternal:
endToEndLatency:
notes:
```

JSON / CSV等を併用しても構いませんが、最終判断の要約はMarkdownへ残します。

## Implementation Checkpoints

両候補を同じCheckpointで止めて比較します。

### P0 — Build Skeleton

完了条件:

- Candidate AがReference PlatformでBuild / Launch
- Candidate Bが同じReference PlatformでBuild / Launch
- 1 Screen表示

ここでは音は不要です。

### P1 — Audio Callback Bring-up

完了条件:

- 実機Audio callbackが動く
- Actual Sample Rate取得
- Actual Callback Frames取得
- SilenceまたはSineを安定出力
- Restart Count取得

### P2 — Reference Audio Core + Diagnostics

完了条件:

- 両候補が同じCoreを呼ぶ
- Callback Load計測
- Audio Frame Timeline
- Variable frameCountでrender可能

### P3 — Realtime Queue + Sample-accurate Trigger

完了条件:

- Bounded Queue
- targetFrame Command
- Buffer内異なるOffsetで複数Trigger
- Queue High-water / Overflow計測

### P4 — Transport / 16-step

完了条件:

- Play / Stop
- BPM
- 16-step Loop
- Lookahead scheduling

### P5 — Minimal Sampler / Oscillator

完了条件:

- One Shot Sample
- Minimal Oscillator
- Poly Voice
- Voice Count / Steal計測

### P6 — Live Pad

完了条件:

- Sequencer Lookaheadを待たない
- Internal latency instrumentation
- Sequencer再生との同時利用

### P7 — Baseline Benchmark

完了条件:

- 32 Voice Scenario
- Boundary Burst
- Callback Load記録
- Dropout / Queue結果記録

### P8 — UI Stress / Lifecycle

完了条件:

- Gesture中Audio安定性
- Interruption / Route / Restart
- Frame Origin再確立
- Pending Command破棄

### P9 — Stress / Stretch

完了条件:

- 10-minute Stress
- 64 Voice Stretch
- 両候補で同形式の結果保存

### P10 — Development Cost Comparison

同じUI変更とDiagnostics変更を両候補へ行い、Build / Debug /変更Layer数を記録します。

### P11 — Technology Decision

`docs/decisions.md`へ以下を記録します。

- 採用Framework / Language / Audio Backend
- Must Gate結果
- Comparative Gate結果
- 採用理由
- 不採用候補の主なTrade-off
- 未解決Risk

その後 `docs/status.md` と `docs/architecture.md` を更新してPhase 1へ進みます。

## 最初のBring-up Platform

Prototypeの最初のCheckpointは、2候補を同じPlatformで比較するため**Androidを第一Bring-up候補**とします。

理由:

- Flutter + Native Audio候補でOboe / AAudio低Latency Pathを直接検証できる
- JUCE候補でもAndroid Audio Backendを比較できる
- Buffer / XRun / Device差の検証価値が高い

ただしこれは製品Platformの優先順位を確定する判断ではありません。

最終Technology Decision前にiOSでもMust要件のSmoke Testを行います。

Reference Deviceの機種はRepositoryへ固定せず、実際に使用するDeviceが決まった時点でResult記録へ残します。

## 次

次の作業はP0 / P1へ入るための実装準備です。

1. `prototypes/` Skeletonを作る
2. Common CoreのBuild方式を決める
3. Flutter Candidateの最小Build Skeletonを作る
4. JUCE Candidateの最小Build Skeletonを作る
5. 同一Android Reference DeviceでLaunch確認する

Framework / SDK Versionは実装開始時点の現行Stableと公式要件を確認して固定し、`docs/decisions.md`またはPrototype Build Notesへ記録します。
