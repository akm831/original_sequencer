# Platform / Audio Backend Requirements

## 目的

Framework / Language / UI Toolkit / Audio Backendを比較する前に、本プロジェクトのAudio要件を満たすための共通評価基準を定義します。

この文書は特定の技術候補を推奨するものではありません。

Flutter、Native、JUCE、Web Technology + Native Wrapper等を比較するときに、「作りやすそう」「人気がある」といった印象ではなく、同じ要件とBenchmarkで評価できる状態を作ることが目的です。

Audio Schedulingの詳細は`docs/audio-engine.md`、Buffer / Latencyは`docs/audio-buffer-latency.md`、Performance Budgetは`docs/performance-budget.md`を正本とします。

## 基本原則

- UI Frameworkの便利さよりRealtime Audioの成立性を優先する
- Sequencer / Project Modelを特定Audio Backendへ密結合させない
- Audio ThreadへUI Runtime、File I/O、Project Model走査を持ち込まない
- Sample-accurateなEvent実行を可能にする
- Backend固有APIはArchitecture境界の内側へ閉じ込める
- MobileではAudio Session / Route Change / Device Restartを通常の運用条件として扱う
- 理論上可能かではなくPrototypeと実機計測で判定する
- 1つのPlatformだけで良好でも、将来対象Platformへ展開できない構成はTrade-offとして明示する

## Must / Should / Nice-to-have

候補比較では要件を3段階に分類します。

### Must

満たせない場合、v0.1の主要候補から外す条件です。

### Should

満たすことを強く望みます。満たせない場合は明確なWorkaround、追加Native Layer、Maintenance Costを示します。

### Nice-to-have

初期版の採否を単独では決めませんが、長期的な開発効率や拡張性で加点します。

## 1. Realtime Audio Callback

### Must

Audio Backendは、Output Audioを生成するRealtime-criticalなCallbackまたは同等の処理境界を提供できること。

Audio処理側で以下を実現可能であること:

- Callbackごとの実際のFrame Countを取得できる
- Sample Rateを取得できる
- Interleaved / Non-interleaved等のBuffer形式を把握できる
- Callback内で任意のBuffer OffsetへEventを反映できる
- Callback Thread上で独自DSPを実行できる
- 不要なAllocation / GC / Blockingを避けられる設計が可能

高Level APIが「音を鳴らす」ことしか提供せず、Buffer単位のAudio Renderingへ十分にアクセスできない構成は不適合とします。

## 2. Sample-accurate Event Scheduling

### Must

Sequencer EventをAudio Buffer先頭へ丸めず、Buffer内のTarget Frame Offsetで処理できること。

例:

```text
callback buffer: frame 10000 ... 10127
event target:    frame 10048

→ offset 48でNote On / Triggerを処理
```

Backend自体に高精度Scheduler APIがなくても、Application側のRealtime Callback内でこの挙動を実装できれば要件を満たせます。

評価時には以下を確認します。

- 同一Buffer内に複数Eventを配置できる
- Event順序をApplication側で決定できる
- Pattern BoundaryとNote Off / Note Onを同一Frameへ配置できる
- Buffer Size変更でSequencer TimingがBuffer境界へ量子化されない

## 3. Variable Buffer Size

### Must

Application内部を特定のBuffer Framesへ固定せず、Backendから渡された実際のFrame Countで処理できること。

Target:

- Preferred: 128 Frames程度
- Stable Fallback: 256 Frames程度
- Compatibility: 512 Frames以上でも破綻しない

64 Frames以下はv0.1必須条件ではありません。

### 評価ポイント

- 128 Framesを要求または優先設定できるか
- 実際に採用されたBuffer Sizeを取得できるか
- Device / Routeによって異なるFramesを扱えるか
- RuntimeのRoute Change等でBuffer条件が変わっても再初期化できるか

## 4. Realtime-safe Threading / Queue

### Must

Scheduler ThreadとAudio Threadを分離できること。

少なくとも以下の構造が実装可能である必要があります。

```text
Project / Sequencer
      ↓
Lookahead Scheduler
      ↓
Bounded Realtime-safe Queue
      ↓
Audio Callback
```

Audio Threadで避けられること:

- Blocking Mutex待ち
- Disk I/O
- Network I/O
- UI Runtimeへの同期Call
- Project Graphの探索
- 大量のLogging
- 予測不能なMemory Allocation / GC Pause

### Should

SPSC Ring Buffer等のBounded Queueを、言語Runtimeの予測不能なPauseへ依存せず実装できること。

Managed Runtimeを使う場合は、Realtime Audio LayerをNative / unmanaged側へ分離できることを評価します。

## 5. Low-latency Live Input Path

### Must

Touch Pad / Touch Keyboard等のLive Inputを、Sequencerの約50 ms Lookahead末尾へ送らず、可能な限り次の安全なAudio処理機会へ渡せること。

```text
Sequencer Event
  → Lookahead Scheduler

Live Input
  → Low-latency Command Path
```

両者は最終的に同じVoice Managerへ到達して構いませんが、Scheduling Policyを分離できなければなりません。

### Benchmark

内蔵Speakerまたは有線Low-latency Routeを使い、Tap / Note OnからAudio開始までのLatencyを実測可能にします。

絶対保証値はBackend選定前には固定しませんが、Touch Instrumentとして不自然な遅延がある候補は不適合とします。

## 6. Audio Session / Device Restart

### Must for Mobile

Mobile候補では以下へ対応できること。

- Audio interruption
- Headphone接続 / 切断
- Output Route Change
- Sample Rate変更
- Callback / Device再初期化

再初期化時にはApplication側で以下を実施できる必要があります。

1. 旧Callbackを安全に停止
2. Pending Audio Commandsを破棄
3. Active Voiceを安全に停止 / 再初期化
4. 新しいSample Rate / Buffer条件を取得
5. Audio Frame Originを再確立
6. Scheduler Queueを再構築
7. TransportとのTime Mappingを再確立

旧DeviceのFrame Timelineをそのまま新Deviceへ持ち越す設計にはしません。

## 7. Audio Clock / Timestamp Access

### Must

Musical TickをAudio Frameへ安定して対応付けるため、Audio処理側に単調増加するFrame Timelineまたは同等のClock情報を構築できること。

Wall ClockだけをTimingの正本にはしません。

Backend APIが絶対Frame Counterを直接提供しない場合でも、Callbackで処理したFramesをApplication側で累積し、安全にOriginを再設定できれば要件を満たします。

## 8. Sampler Playback / Decode Thread Boundary

### Must

Audio Callback内でSample Fileを開いたり、圧縮FileのDecode開始を行わない構造を実装できること。

Sample ResourceはAudio Threadへ渡す前に準備済みであることを基本とします。

### Should

以下をAudio Thread外へ分離できること。

- File Open
- Metadata Parse
- Decode
- Waveform Peak Cache生成
- 将来のStreaming Buffer Refill

短いOne Shot SampleをMemoryへ保持する方式と、将来の長尺Sample Streamingを両方拡張可能な構造を評価します。

Streaming / Cachingの具体仕様は別Topicで決定します。

## 9. Custom DSP Extensibility

### Must

Built-in Synth、Sampler Pitch、Filter、Envelope、Mixer等をApplication側で実装・拡張できること。

OS標準Player NodeやMedia APIだけに依存し、独自Voice DSPをRealtimeにRenderできない構成は対象外とします。

### Should

以下を無理なく実装できること。

- 2 OSC + Noise Synth
- Polyphonic Filter / Envelope
- Sampler Pitch / Resampling
- Parameter LockによるTrigger-local State
- 将来のDelay / Reverb等

## 10. Performance Benchmark Capability

### Must

候補Backend上で、共通Reference Benchmarkを実行しDiagnosticsを取得できること。

最低限取得したい情報:

- Sample Rate
- Callback Buffer Frames
- Callback Processing Time
- Callback Load
- Peak / Percentile Load
- Underrun / Dropout Countまたは検出可能な代替指標
- Active Voice Count / Peak
- Voice Steal Count
- Queue High-water Mark
- Queue Overflow Count

### Baseline Target

48 kHz / 128 Frames程度をPreferred条件として、32 Concurrent Voicesを含むReference Projectを安定再生できることを最低Performance Target候補とします。

### Stretch Target

64 Light-to-Moderate Voicesを安定再生できること。

64 Heavy Synth Voicesを全Deviceで保証する要件ではありません。

### Callback Headroom

Reference ProjectではCallback Load概ね50%以下をNormal Targetとします。

継続的に70〜80%以上へ張り付く候補はHeadroom不足として評価します。

## 11. Benchmark Scenarios

すべての有力候補を同じScenarioで比較します。

### A. Sampler Heavy

- 約8 Tracks
- 合計32 Voices前後
- One Shot Tail overlap
- Pitch変換あり / なし

### B. Synth Heavy

- 複数Synth Tracks
- 2 OSC + Noise
- Filter
- Amp / Filter Envelope
- 16〜32 Synth Voices

### C. Mixed Groovebox

- Drum Sampler
- Bass Synth
- Chord / Pad Synth
- Sample Tail
- Mixer Level / Pan
- Parameter Lock相当のTrigger State更新
- Pattern Switching

### D. Stress / Burst

- Pattern Boundaryで同時Trigger
- Chord + Release Tail overlap
- Parameter Lockを伴うTrigger Batch
- Live Input追加

正式比較では10分程度のStress Playbackを共通基準候補とします。

## 12. Platform Integration

### Mobile Must / Should

- iOS Audio Session相当の制御へアクセス可能
- Android Low-latency Audio Path相当へアクセス可能
- Background / interruption / route changeのPlatform EventをApplicationへ伝達可能
- Bluetooth等の高Latency Routeを識別または少なくとも挙動差を観測可能

### Desktop Should

将来Desktop対応を行う場合に、Platform固有Audio APIまたは安定したCross-platform Backendへ接続可能であること。

Desktop対応をv0.1必須にするかはPlatform選定時に別途決定します。

## 13. FrameworkとAudio Backendの分離

UI FrameworkとAudio Backendを同一技術へ統一すること自体は要件にしません。

例えば、UIがManaged / Cross-platform Frameworkでも、Realtime AudioをNative Layerへ分離する構成は許容します。

重要なのは以下です。

- Sequencer CoreがUI Toolkitへ依存しない
- Audio CallbackがUI Runtimeへ依存しない
- Command / State境界が明確
- Native Bridge CostがAudio Callback Deadlineへ入らない

Audio Callbackごとに大量のDart / JavaScript / UI-runtime Bridgeを往復する構成は避けます。

## 14. Language / Runtime評価

Framework比較時にはProgramming Languageそのものより、Realtime領域での実行特性を確認します。

評価項目:

- GC Pauseの有無 / 制御可能性
- Audio Callback内Allocationを避けられるか
- SIMD / Native DSP利用性
- Lock-free / wait-free data structureの実装性
- Native LibraryとのInterop Cost
- Debug / ProfilerでRealtime Bottleneckを追跡できるか

Managed Languageを採用しても、Audio CallbackをNative Layerへ閉じ込めれば成立する場合があります。

したがって「GC言語だから即不採用」とはせず、Realtime境界をどこへ置くかで評価します。

## 15. Web系構成の追加評価

Web Technologyを候補に含める場合、通常のWeb Audioが利用可能というだけでは合格にしません。

追加で確認すること:

- Mobile App Wrapper内で安定した低Latency Audio Pathを確保できるか
- AudioWorklet等のRealtime処理境界の制約
- Native Audio Layerを併用する場合のBridge構造
- Background / interruption / route change対応
- File Import / Asset Access
- 128 Frames級の実動作条件
- 32 Voice Baseline Benchmark

Browser単体での動作性と、App Store / Play Store向けNative AppとしてのRealtime性能は分けて評価します。

## 16. JUCE / Native系構成の追加評価

Native / C++ / JUCE系はRealtime Audio面で有力でも、以下のTotal Costを比較します。

- Touch-first UI開発速度
- iOS / Android UI Integration
- Platform Permission / File Picker
- Accessibility
- Text / Layout / Localization
- Build / Debug複雑性

Audio性能だけでFramework全体を決めず、必要ならUI LayerとAudio Layerを分離します。

## 17. Prototypeで必ず確認する項目

候補を最終決定する前に、少なくとも小さなVertical Sliceを作ります。

Prototype範囲:

- Play / Stop
- BPM
- 16-step Sequencer
- 1〜複数Tracks
- Timestamped Event Queue
- Buffer内Sample Offset Trigger
- Simple SamplerまたはSimple Synth
- Live Pad Trigger
- Diagnostics表示 / Log

評価:

- Timing Jitter
- Live Input Latency
- Callback Load
- Dropout
- Route Change / Restart
- 128 / 256 Framesでの安定性
- 32 Voice Baseline

候補技術の比較記事や理論値だけで最終決定しません。

## 18. Candidate Scorecard

候補比較時は以下の表を埋めます。

| 評価項目 | Weight | Candidate A | Candidate B | Candidate C |
|---|---:|---|---|---|
| Sample-accurate Scheduling | Must | | | |
| Realtime-safe Callback | Must | | | |
| 128 / Variable Buffer | Must | | | |
| Low-latency Live Input | Must | | | |
| Device Restart / Route Change | Must Mobile | | | |
| Custom DSP | Must | | | |
| 32 Voice Benchmark | Must | | | |
| 64 Voice Stretch | Should | | | |
| Native Bridge Simplicity | Should | | | |
| Touch-first UI Productivity | High | | | |
| iOS / Android Coverage | High | | | |
| Desktop Expansion | Medium | | | |
| DSP Debug / Profiling | High | | | |
| Build / Maintenance Cost | High | | | |
| Community / Library Maturity | Medium | | | |

Must項目に重大な欠落がある候補は、合計Scoreが高くても原則として採用しません。

## 19. 現時点で固定しないこと

この段階では以下を決定しません。

- Framework
- Programming Language
- UI Toolkit
- Audio Backend製品名
- iOS / Android / Desktopの最終優先順位
- C++ Layerが必須か
- Flutter / Native / JUCE / Webの採否

まず本Requirementで候補を比較し、その後にTechnology Decisionを行います。

## v0.1選定時の合格条件

有力候補は最低限以下を満たす必要があります。

- Realtime Audio Callbackまたは同等の低Level Processing境界を持つ
- Buffer内Sample OffsetでEventを実行できる
- 128 Frames程度をPreferred Targetとして試験でき、Variable Bufferへ対応できる
- SchedulerとAudio ThreadをRealtime-safe Queueで分離できる
- Live InputをSequencer Lookaheadから分離できる
- MobileではAudio Session / Route Change / Restartへ対応できる
- Audio Thread外でSample Decode / File I/Oを行える
- 独自Sampler / Synth DSPを実装できる
- 32 Concurrent Voices BaselineをReference Benchmarkで検証できる
- Callback Load / Dropout等を診断可能
- Sequencer CoreをBackend / UIから分離できる

## 次の検討

- Flutter + Native Audio構成
- Fully Native構成
- JUCE / C++中心構成
- Web Technology + Native Audio Wrapper構成
- 各候補の実装複雑性、性能、Platform Coverage比較
- Technology Prototypeの最小Scope決定
