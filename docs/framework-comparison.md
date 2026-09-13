# Framework / Language / Audio Backend候補比較

## 目的

`docs/platform-audio-requirements.md`で定義したMust / Should / Benchmarkを基準に、Phase 1へ進む前のTechnology候補を比較します。

この文書は最終採用技術を確定するものではありません。

最終判断は、ここで絞り込んだ候補を同じVertical Slice / Reference Benchmarkで実機比較してから行います。

確認日: 2026-09-13

## 比較対象

1. Flutter UI + Native / C++ Audio Layer
2. Fully Native（iOS / Android個別UI + Native Audio）
3. JUCE / C++中心
4. Web Technology + Native Mobile Wrapper

## 共通前提

どの候補でも次のArchitecture境界は維持します。

```text
UI / Interaction
      ↓
Application Commands
      ↓
Sequencer / Transport
      ↓
Scheduler
      ↓
Bounded Command Queue
      ↓
Realtime Audio Engine
      ↓
Platform Audio Backend
```

Realtime Audio CallbackはUI Runtimeへ同期Callしません。

特にManaged / Web UIを使う場合、BridgeをCallback Deadlineへ入れず、UI → Native CommandとNative → UI State Snapshotの低頻度境界として扱います。

## 評価記号

- ◎: 要件に自然に適合し、有力
- ○: 適合可能。追加設計やPlatform Adapterが必要
- △: 実現可能性はあるが、Prototypeで重点検証が必要
- ×: v0.1主要候補として不適合

## 比較表

| 評価項目 | Flutter + Native Audio | Fully Native | JUCE / C++ | Web + Native Wrapper |
|---|---:|---:|---:|---:|
| Realtime Callback / Custom DSP | ◎ | ◎ | ◎ | △ |
| Sample-accurate Buffer内Event | ◎ | ◎ | ◎ | △ |
| 128 / Variable Buffer対応 | ◎ | ◎ | ◎ | △ |
| Realtime-safe Queue構築 | ◎ | ◎ | ◎ | △ |
| Live Input Low-latency Path | ◎ | ◎ | ◎ | △ |
| Mobile Audio Session / Restart | ○ | ◎ | ○〜◎ | △ |
| Audio Clock / Frame Timeline | ◎ | ◎ | ◎ | ○ |
| Sampler Decode境界 | ◎ | ◎ | ◎ | ○ |
| Built-in Synth / DSP拡張 | ◎ | ◎ | ◎ | △〜○ |
| 32 Voice Benchmark実施性 | ◎ | ◎ | ◎ | △ |
| Touch-first UI開発速度 | ◎ | ○ | △〜○ | ◎ |
| iOS / Android UI共有 | ◎ | × | ○ | ◎ |
| Desktop拡張 | ◎ | △〜○ | ◎ | ◎ |
| Native Bridge複雑性 | ○ | ◎ | ◎ | △ |
| Build / Debug単純性 | ○ | △ | ○ | △ |
| 長期Maintenance Cost | ○〜◎ | △ | ○ | △ |

## 1. Flutter UI + Native / C++ Audio Layer

### 想定構成

```text
Flutter / Dart
- Touch UI
- Navigation
- Editor Views
- Project操作
        ↓ FFI / Platform Integration
Native Shared Layer
- Scheduler境界
- Realtime Command Queue
- Sampler / Synth DSP
- Voice Manager
- Mixer
        ↓
Platform Adapter
- iOS Audio API
- Android Oboe / AAudio系
```

Flutter / DartをAudio Callbackの実行環境にはしません。

### 強み

- Touch-first UIをiOS / Androidで共有しやすい
- Dart FFIでNative C APIを呼び出せる
- C / C++ Audio CoreをFlutter UIから分離できる
- Audio Callback内をNative側へ閉じ込めればGC / UI FrameとRealtime Deadlineを分離できる
- 将来Desktop UIへ展開しやすい
- UIのIteration速度とAudio EngineのRealtime制御を両立しやすい

### 注意点

- Dart ↔ NativeのCommand / State Contract設計が必要
- Audio CallbackごとのBridge往復は禁止する
- iOS / AndroidのAudio Session、Route Change、Device RestartはPlatform Adapterが必要
- Debug時にDart / C++ / Platform Nativeの複数Layerを追う必要がある
- Flutter Plugin / FFI packagingを早期に検証する必要がある

### 判定

**Prototypeへ進める第一候補。**

本プロジェクトの「Touch-first UI」と「Realtime Audio分離」の両方に最もバランス良く適合する可能性があります。

ただし採用確定ではなく、Bridge Cost、Lifecycle、実機Latency、32 Voice Benchmarkを確認します。

## 2. Fully Native

### 想定構成

```text
iOS
Swift / SwiftUI or UIKit
+ Native Audio

Android
Kotlin / Compose
+ C++ / Oboe Audio
```

必要であればDSP CoreのみC++共有とします。

### 強み

- Platform Audio Session / Lifecycleへのアクセスが最も直接的
- Native Profiler / Debuggerをそのまま使いやすい
- UIとPlatform固有挙動のIntegrationで制約が少ない
- Bridge Layerを最小化できる

### 弱み

- iOS / Android UI実装が分かれやすい
- Touch InteractionやEditorを二重に保守するCostが大きい
- Feature追加ごとのPlatform差分が増えやすい
- 小規模開発でPhase 1〜5を高速に回すには負担が大きい

### 判定

**性能 / Platform IntegrationのReferenceとして強いが、現時点の第一候補にはしない。**

Flutter + Native AudioがLifecycleやLatency要件を満たせない場合のFallbackとして保持します。

## 3. JUCE / C++中心

### 想定構成

```text
JUCE Application
- C++ UI
- Sequencer
- AudioDeviceManager / Audio Backend
- Sampler / Synth DSP
- Mixer
```

必要に応じてPlatform固有処理を追加します。

### 強み

- Audio / DSP中心のC++ Frameworkとして成熟している
- Windows / macOS / Linux / iOS / Androidを同一Codebaseから狙える
- Audio Device、DSP、MIDI等の機能が同じFramework内に揃う
- Realtime CallbackとCustom DSPの構築が自然
- Desktop拡張や将来のPlugin関連と相性が良い
- AndroidではOboe Backendを利用可能

### 注意点

- スマートフォン向けTouch-first UIをどこまで素早く洗練できるか検証が必要
- Mobile Native UI慣習とのIntegration Costを確認する必要がある
- UIを含めC++比率が高くなり、Product UIのIteration速度がFlutterより落ちる可能性がある
- Framework更新とPlatform SDK差分をC++ Build全体で扱う必要がある

### 判定

**Prototypeへ進める第二候補。**

Audio要件だけなら非常に有力です。

Flutter候補との比較では、Audio性能差だけでなく「同じTouch-first画面を作る時間」「Gesture / Layout変更のしやすさ」も計測します。

## 4. Web Technology + Native Mobile Wrapper

### 想定構成

```text
HTML / CSS / JavaScript UI
        ↓
WebView / Hybrid Wrapper
        ↓
Web Audio AudioWorklet
または
Native Audio Plugin
```

### 強み

- UI開発速度が高い
- Web技術の人材 / Toolingが豊富
- AudioWorkletにはAudio Rendering Thread上のBlock処理境界があり、現在一般的なRender Quantumは128 Frames
- `currentFrame`等、Sample Frame基準の情報も取得可能

### リスク

- Browser / WebView / OS / WrapperのLayerが増える
- Mobile AppとしてのAudio Session、Route Change、Background挙動をWeb APIだけでは完結しにくい
- Native Audio Pluginを本格採用すると、結局Web UI + Native Audioの二層Architectureになり、Bridge / Lifecycleの不確実性が増える
- Browserで動くことと、低Latency Touch InstrumentとしてApp Store / Play Storeで安定することは別問題
- 32 / 64 Voice Benchmarkと端末差を早期に潰す必要がある

### 判定

**v0.1主要候補からは一旦外す。**

将来Web版を作る価値はありますが、Mobile Grooveboxの最初の実装基盤としては、Flutter + Native AudioやJUCEより検証項目が多くなります。

## Native Audio Backend方針

Framework選択とは別に、Mobile Audio BackendはNative Low-latency Pathを優先します。

### Android

有力候補はOboeです。

Googleの現行GuidanceではHigh-performance / Low-latency Audio向けにOboeまたはAAudioを推奨しており、Oboeは新しいAndroidではAAudioを使用し、必要に応じて旧Pathを吸収します。

Prototypeでは少なくとも以下を検証します。

- LowLatency Performance Mode
- Callback Rendering
- 実際のFrames Per Burst / Callback Frames
- XRun / Underrun
- Device Disconnect / Restart
- 48 kHz系の実動作

### iOS

AVAudioSession等を使い、実際のSample Rate / I/O Buffer Duration / Route Change / InterruptionをNative Layerで扱えることを前提とします。

Preferred値を要求しても実際の値と一致する保証はないため、Audio Logicは実測値を読み取りVariable Bufferとして処理します。

## 現時点の順位

### Prototype対象

1. **Flutter UI + Native / C++ Audio Layer**
2. **JUCE / C++中心**

### Reference / Fallback

3. **Fully Native**

### 保留

4. **Web Technology + Native Wrapper**

これは製品Stackの最終決定ではありません。

## 次のTechnology Prototypeで比較すべきこと

候補1と候補2について、同じ最小Vertical Sliceを作ります。

### 共通Scope

- 1 Screen
- Play / Stop
- BPM
- 16-step Grid
- 1 Sampler Track
- 1 Synth Trackまたは簡易Oscillator
- Sample-accurate Trigger
- Live Pad Trigger
- Callback Frame Count表示
- Callback Load計測
- Underrun / XRun相当の計測
- Active Voice Count表示
- Pattern Boundary相当の同時Trigger Stress

### 重要な比較値

- Tap → Audio開始の体感 / 実測Latency
- 48 kHz / Preferred 128 Frames条件への到達性
- 256 Frames Fallback時の安定性
- 32 Concurrent VoicesでのCallback Load
- 64 Light Voice Stretch
- 10分Stress PlaybackでのDropout
- Route Change / App interruption復帰
- UI Gesture中のAudio Stability
- Native Bridge Message Rateを増やした際のAudio Stability
- 同じ画面を実装 / 修正するDevelopment Cost
- Device上でのDebugしやすさ

## Prototypeで避けること

Technology比較の段階では次を作り込みません。

- 完成版Project Persistence
- 高度なSampler Editor
- Full Synth UI
- Probability / Ratchet / Conditional Trigger
- Production-ready Effects
- Factory Content管理
- Song Mode

比較目的に不要な機能を増やさず、Realtime Audio成立性とTouch UI開発効率を測定します。

## 採用判断のGate

最終候補は以下を満たす必要があります。

1. `docs/platform-audio-requirements.md`のMustを満たす
2. 32 Voice Baselineで安定する
3. Audio CallbackがUI Runtimeから独立する
4. Live InputがSequencer Lookahead待ちにならない
5. Device / Route Restartから復帰できる
6. Touch UIのIteration Costが許容範囲
7. Build / Debug / DistributionのMaintenance Costが許容範囲

64 Voice Stretchは加点項目であり、全Device必須条件にはしません。

## 参考資料

Technology固有の事実確認には一次資料を優先します。

- Flutter: https://docs.flutter.dev/platform-integration/bind-native-code
- Flutter Packages / Plugins: https://docs.flutter.dev/packages-and-plugins/developing-packages
- JUCE Features: https://juce.com/features/
- JUCE AudioDeviceManager: https://juce.com/tutorials/tutorial_audio_device_manager/
- Android High-performance Audio: https://developer.android.com/ndk/guides/audio
- Android Oboe Low-latency Audio: https://developer.android.com/games/sdk/oboe/low-latency-audio
- Apple AVAudioSession I/O Buffer Duration: https://developer.apple.com/documentation/avfaudio/avaudiosession/iobufferduration
- Web Audio AudioWorkletProcessor: https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletProcessor/process
- Web Audio currentFrame: https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletGlobalScope/currentFrame
