# Audio Engine / Scheduling仕様

## 目的

Sequencerが扱うMusical Timeを、実際のAudio Outputで正確に発音できるAudio Timeへ変換する責務を定義します。

この文書は特定のFramework、Programming Language、Audio Backendへ依存しません。

v0.1では、UIやSequencer LogicがAudio Callbackを直接操作せず、Transport / SchedulerとRealtime Audio Threadの間に明確な境界を置きます。

## 基本原則

- SequencerはMusical Eventを生成する
- Musical Positionは960 PPQNを基準とする
- SchedulerはEventをAudio Timeへ変換する
- Audio Callbackは現在のBuffer内に入るEventをSample Offset単位で実行する
- Audio ThreadではFile I/O、Blocking Lock、UI参照、可変Project Modelの走査を避ける
- Sampler / Synthは共通Event Contractを受け取るが、Voice内部DSPは各Engine固有とする

概念:

```text
UI / Project Model
        ↓
Sequencer
        ↓
Transport / Musical Clock
        ↓
Lookahead Scheduler
        ↓
Timestamped Audio Commands
        ↓
Realtime-safe Queue
        ↓
Audio Callback
        ↓
Sampler / Synth Voice Manager
        ↓
Mixer / Output
```

## 2種類の時間

### Musical Time

Sequencer側では音楽的な位置をTickで扱います。

現在の設計目標:

- 960 PPQN
- 4/4
- v0.1 Main Gridは1 Step = 1/16 note
- 1 Step = 240 ticks

Eventの最終Musical Positionは概念的に以下です。

```text
musicalTick
= gridTick
+ swingOffsetTicks
+ microTimingTicks
```

### Audio Time

Audio Engine側では、最終的にSample / Frame位置へ変換します。

概念:

```text
targetFrame = musicalTickToAudioFrame(...)
```

変換には少なくとも以下が関係します。

- BPM
- PPQN
- Sample Rate
- Transport Start基準
- Tempo変更（将来）

UIやStep ModelへAudio Frameを保存しません。

## Lookahead Scheduler

Audio CallbackがStep境界のたびにProject Modelを探索する方式は採用しません。

SchedulerはRealtime Audio Threadの外で、少し先のMusical Eventを事前に評価します。

担当:

1. Transport位置からLookahead範囲を決める
2. 対象Stepを評価する
3. Probability / Condition等を評価する（実装時）
4. Swing / Micro Timingを反映する
5. Note On / Note Off等のMusical Eventを作る
6. Parameter Lock等のEvent-local Overrideを解決する
7. Musical TimeをAudio Timeへ変換する
8. Timestamp付きCommandとしてRealtime Queueへ送る

Lookaheadの具体的な長さはAudio Buffer / Latency仕様で後から決定します。

## Timestamped Audio Command

Audio Threadへ渡すCommandは、可変Project ModelへのPointer参照ではなく、実行に必要な小さく安定したPayloadを持つことを基本とします。

概念例:

```text
AudioCommand
├─ targetFrame
├─ type
├─ trackId / engineInstanceId
├─ originPatternId
├─ eventId / triggerBatchId
└─ payload
```

初期Command候補:

- TriggerBatch / NoteOnBatch
- NoteOff
- PatternBoundaryTransition
- TransportStop
- AllNotesOff
- AllVoicesStop / Panic
- future: RuntimeParameterChange

Backend実装時にCommand Type名は変更可能ですが、意味はこの仕様を維持します。

## Trigger Batch

ChordやPitched Samplerの複数Noteは、同一時刻の1つのTrigger Batchとして扱えるようにします。

例:

```text
notes = [C3, E3, G3, Bb3]
triggerBatchId = X
```

これによりSamplerのChoke判定をBatch開始前に一度だけ行い、その後に4 Voiceを生成できます。

同一Batch内のVoice同士はChoke対象にしません。

## Sample-accurate Scheduling

Audio CallbackがN FramesのBufferを処理するとき、そのBuffer内のEventをBuffer先頭からのOffsetへ変換して適用します。

例:

```text
Buffer: frame 10000 ... 10511
Event : frame 10240

→ bufferOffset = 240
```

EventをBuffer開始時へ丸めず、可能な限り`bufferOffset = 240`の位置で発音します。

これを本プロジェクトにおけるSample-accurate Schedulingの基本目標とします。

これにより、Buffer Sizeが大きくなってもStep TimingそのものがBuffer境界へ量子化されにくくなります。

## Audio Callbackの責務

Audio CallbackはRealtime-criticalな領域として扱います。

主な責務:

- 現在Buffer範囲のCommandを取得する
- CommandをtargetFrame順に処理する
- 必要なVoiceを開始 / Release / Stopする
- 各VoiceをBufferへRenderする
- Mixer処理を行う

避けること:

- Disk / Network I/O
- Sample File decode開始
- Project JSONの読み書き
- UI State参照
- 長時間BlockingするMutex取得
- 不要なMemory Allocation / Deallocation
- 可変Project Graphの複雑な探索

具体的なRealtime-safe実装方式はBackend選定後に確定します。

## Queue境界

SchedulerからAudio Callbackへの受け渡しには、Realtime用途に適したQueueを使用します。

初期設計では、単一Producer / 単一Consumerにできる場合はSPSC Queueを優先候補とします。

```text
Scheduler Thread  --producer-->  Queue  --consumer--> Audio Thread
```

複数Producerが必要な設計になった場合は、Audio Thread側へBlockingを持ち込まない別方式を検討します。

Queue実装そのものはFramework / Language選定後に決定します。

## Parameter Lockの解決境界

Stepに保存されたParameter Lockは、Audio ThreadがProject Modelを検索して解決しません。

原則:

```text
Track Engine Default
       +
Step Parameter Locks
       +
Event-local Override
       ↓
Resolved Trigger State
       ↓
Audio Command / Voice Creation
```

VoiceのLifetime中に固定されるParameterは、Trigger時点のResolved StateとしてVoiceへ渡します。

SamplerのSample Start / Pitch / Reverse等はこの方式を基本とします。

将来Continuous Automationを追加する場合は、Trigger Snapshotとは別にTimestamp付きRuntime Parameter Changeを定義します。

## Pattern切替Scheduling

演奏中のPattern切替は既存仕様どおり次のProject Bar Boundaryで行います。

SchedulerはPattern選択直後にAudio ThreadのStateを直接変更せず、切替境界をMusical Time上で確定します。

境界では概念的に以下を同じAudio Timeへ配置します。

1. 旧Pattern由来のGate-held Voiceへの終了処理
2. Pattern / Track Phaseの切替
3. 新PatternをStep 1から開始
4. 同Boundaryに存在する新Pattern Triggerを実行

Samplerの扱い:

- Gate → Note Off / Release
- Loop → Note Off / Release
- One Shot → 原則Carry

SynthのGate-held NoteもNote Off / Releaseを基本とします。

旧Pattern由来の自然なRelease TailはVoiceとして残せます。

## 同一FrameのCommand順序

同じAudio Frameに複数Commandが存在する場合、結果をBackend依存にしないよう明示的なOrderingを持ちます。

v0.1の基本Priority:

1. Panic / Explicit All Voices Stop
2. Pattern Boundary等による旧GateのNote Off
3. 通常Note Off
4. Trigger Batch開始時のChoke
5. 新しいNote On / Voice Allocation
6. Voice Stealing（Voice Allocation時に必要な場合のみ）

重要:

- Chokeは新Voice生成前に処理する
- Voice StealingはChokeや終了済みVoice整理後、それでもSlot不足の場合のFallbackとする
- 同一FrameのNote Off → Note Onは、同PitchのRetriggerを予測可能にするためこの順序を基本とする

Mono SynthのLegatoでは別Semanticsが必要になる可能性があるため、Engine固有Policyとして拡張可能にします。

## Common Voice Contract

Sampler / Synth VoiceのDSP実装を同一Classへ統合する必要はありません。

Voice Managerが共通に扱う最小概念のみ揃えます。

概念:

```text
Voice
├─ voiceId
├─ engineInstanceId
├─ originPatternId
├─ originTrackId
├─ originEventId / triggerBatchId
├─ startedAtFrame
├─ lifecycleState
│   ├─ active
│   ├─ release
│   ├─ stopping
│   └─ finished
├─ note identity (if applicable)
└─ engine-specific render state
```

共通操作:

- begin / create
- noteOff / release
- forceStopWithDeClick
- isFinished
- age / startedAt参照

Sampler固有:

- Sample Resource
- Playback Position
- Loop / One Shot / Gate
- Choke Group

Synth固有:

- Oscillator Phase
- Filter State
- Envelope State
- Mono / Poly / Glide Behavior

この境界により、Voice Lifecycle Policyを共有しつつDSP実装は独立できます。

## Polyphony Budget

Polyphonyは2段階で扱います。

### Musical / Engine Limit

TrackまたはEngineが持つ音楽的な上限です。

現時点の暫定値:

- Sampler: 最大8 Voices / Track
- Synth Poly: 最大8 Voices / Track
- Synth Mono: 1 Voice

この上限を超える場合はEngine / Track単位のVoice Stealingを行います。

### Global Safety Budget

Audio Engine全体には、CPU / Memory保護のためのGlobal Voice Budgetを持てる構造にします。

ただし具体的な数値はまだ決定しません。

理由:

- Platform
- Device性能
- Sample Rate
- Buffer Size
- Synth DSP負荷
- Effect負荷

に依存するためです。

Prototype / 実機計測後に決定します。

Global Budgetへ到達した場合のStealing Policyも実測後に最終決定します。

## Voice Stealingの共通Default

Polyphonic Engineでは、特別なEngine固有Ruleがなければ以下をDefault候補とします。

1. Release中の最古Voice
2. それ以外の最古Voice

SamplerはこのRuleをv0.1仕様として採用済みです。

Synth Polyも初期Defaultとして同じRuleを使用できる設計にしますが、Mono / Legatoや将来の特殊SynthはEngine固有Policyで上書き可能にします。

## Stopの種類

停止Commandは意味を区別します。

### Note Off

通常の音楽Event終了。

- Sampler Gate / Loop → Release
- Sampler One Shot →通常は無視
- Synth → Release

### All Notes Off

Gate-held NoteをReleaseへ移行するための音楽的停止。

Release Tailは残せます。

### All Voices Stop / Panic

緊急または明示的な完全停止。

One ShotやRelease Voiceを含め、対象Voiceを短いDe-click処理の後に終了できます。

### Transport Stop

通常のUser Stopです。

v0.1ではSequencer由来Voiceを停止対象とします。

具体的なTailの残し方はEngine Semanticsに従いますが、長いOne ShotがStop後も無制限に鳴り続けないようにします。

## SchedulerとProject編集

再生中にUIからProject Stateが変更されても、Audio Threadが可変Modelを直接読む方式にはしません。

変更はApplication / Sequencer側で受け取り、必要なものだけ今後のScheduled Eventへ反映します。

既に生成済みのVoiceは、原則としてTrigger時のResolved Stateを維持します。

将来Continuous AutomationやLive Parameter Controlを追加する場合は、専用のRealtime Parameter Command経路を追加します。

## v0.1で確定するScheduling Scope

- Musical Timeは960 PPQN Tickを基準にする
- SchedulerとAudio Callbackを分離する
- SchedulerはLookaheadでEventを準備する
- Audio ThreadへTimestamp付きCommandを渡す
- Buffer内Eventは可能な限りSample Offset位置で実行する
- Audio ThreadはProject Model、UI、File I/Oへ依存しない
- Parameter LockはAudio Thread外でEvent-localなResolved Stateへ解決する
- Pattern切替はBar BoundaryのScheduled Eventとして扱う
- ChokeはVoice Allocation前に処理する
- Voice Stealingは必要な場合だけAllocation Fallbackとして行う
- Common Voice ContractはLifecycleを共有し、DSP内部はEngine固有とする
- Sampler / Synth Polyは1 Track 8 Voicesを暫定上限とする
- Global Voice Budgetの具体値は実機計測後に決定する

## 未決事項

次に検討する内容:

- Audio Buffer Size
- Target Latency
- Lookahead Length
- Queue容量とOverflow Policy
- Audio Device Start / Restart時のClock同期
- Sample Rate変更時の扱い
- Global Voice Budgetの具体値
- Global Budget到達時のVoice Stealing Policy
- Synth Mono Legato / Retrigger Semantics
- Live Parameter Change / Continuous Automationの補間
- Resampling / Interpolation Quality
- Audio Backend候補ごとのSample-accurate Scheduling実現性
