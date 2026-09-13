# Voice Management仕様

## 目的

Sampler / Synth等のAudio Engineで、Triggerから生成された発音単位（Voice）の寿命、重なり、停止、Polyphony制限を一貫して扱います。

この文書ではまずSampler v0.1に必要なVoice Semanticsを確定し、将来Synthを含む共通Voice Managementへ拡張できる境界を定義します。

VoiceはRuntime Stateであり、Projectへ永続化しません。ProjectにはEngine StateとSequencer Eventを保存し、再生時にそれらからVoiceを生成します。

## 基本モデル

Sequencerは音声そのものを直接管理せず、音楽EventをAudio Engineへ渡します。

概念:

```text
Sequencer Event
      ↓
Sampler Engine
      ↓
Voice Manager
      ↓
Sampler Voice(s)
      ↓
Mixer
```

Samplerの1回のTriggerは、必要な数の新しいVoiceを生成します。

Pitched Samplerで`notes[]`に複数Noteがある場合は、各Noteが独立したVoiceになります。

例:

```text
notes = [C3, E3, G3, Bb3]
↓
4 Sampler Voices
```

## VoiceはTrigger時点の状態を保持する

再生中Voiceは、現在のTrack / PatternのEngine Stateを毎Sample参照し続けるのではなく、Trigger時点で必要な有効状態を保持します。

概念:

```text
SamplerVoice
├─ sourceAssetId
├─ sourceRange
├─ playbackDirection
├─ playbackMode
├─ pitch
├─ ampEnvelopeState
├─ filterState
├─ chokeGroup
├─ originPatternId
├─ originTrackId
├─ originEventId / trigger identity
├─ startedAt
└─ runtime playback position
```

Parameter LockやSample Overrideがある場合、それらをTrackのDefault Stateへ適用した結果をVoice生成時に解決します。

これにより、Pattern切替後に旧Pattern由来のOne Shotが鳴り残っても、新PatternのSampler設定へ突然変化しません。

将来Continuous Automationを追加する場合は、SnapshotだけでなくRuntime Parameter UpdateをVoiceへ送る経路を別途定義します。

## Playback Mode

### One Shot

TriggerでVoiceを開始し、指定されたSample RangeのEndへ到達するまで再生します。

通常のNote Off / Step Lengthでは停止しません。

```text
Note On  -> start
Note Off -> ignore for source lifetime
End      -> voice finishes
```

用途:

- Kick
- Snare
- Percussion
- FX Hit
- 短いPhrase

Amp Envelopeは音量形成に使用できますが、通常のNote OffはOne Shotの終了条件にしません。

ただし以下はOne Shotを強制終了できます。

- Choke
- Voice Stealing
- Explicit All Voices Stop
- Asset / Engineの致命的な再生Error

強制終了時は可能な限り短いDe-click Fadeを適用し、波形を不連続に切ってClickを発生させないようにします。

### Gate

TriggerでVoiceを開始し、Note / GateがActiveな間再生します。

Note Offを受けるとAmp EnvelopeのReleaseへ移行します。

```text
Note On  -> attack / playback start
Gate On  -> playback continues
Note Off -> release
Silence or End -> voice finishes
```

Sample Endへ先に到達した場合は、その時点でVoiceを終了できます。

用途:

- Melodic Sample
- Vocal Fragment
- Bass Sample
- Chopped Phrase

### Loop

TriggerでVoiceを開始し、VoiceがActiveな間はLoop Regionを繰り返します。

v0.1ではSamplerのStart / EndをLoop Regionとして兼用します。

Note Offを受けるとAmp EnvelopeのReleaseへ移行し、Release中も必要に応じてLoop再生を継続します。Envelopeが実質的な無音へ到達した時点でVoiceを終了します。

これによりNote Off直後にLoop波形をHard Cutせず、Amp Releaseを自然に反映できます。

将来専用Loop Start / End、Loop Crossfade、Ping-Pong等を追加する場合も、このVoice Lifetime Modelは維持します。

## 同じSampleのRetrigger / Overlap

v0.1の標準動作は**Overlapを許可**します。

同じTrack、同じSample、同じPitchであっても、新しいTriggerは既存Voiceを暗黙には停止せず、新しいVoiceを生成します。

例:

```text
Kick Step 1 -> Voice A
Kick Step 3 -> Voice B
```

Step 3でVoice Aがまだ再生中でも、Voice Bを開始できます。

理由:

- Drum / Percussionの自然なTailを維持できる
- Clap、Cymbal、FX等の重なりを表現できる
- Pitched SamplerのPolyphonyと同じ基本モデルにできる
- Retrigger専用の例外RuleをCoreへ持ち込まずに済む

意図的に前の音を切りたい場合はChoke Groupを使用します。

将来、Mono Samplerや明示的なRetrigger Modeが必要になった場合は追加可能ですが、v0.1の必須機能にはしません。

## Sampler Polyphony

v0.1では**1 Sampler Trackあたり最大8 Voices**を暫定上限とします。

これは次を両立するための初期値です。

- 初期Harmonyの最大4音程度のChord
- Drum / FX TailのOverlap
- Mobile環境での予測可能なCPU / Memory使用量
- Synthの暫定8 Voice目標との整合

Polyphonyは「同時にActiveなVoice数」です。

4音Chordは4 Voicesを使用します。

Global Maximum PolyphonyはPlatform / Audio Backend選定と実機計測後に決定します。

8 Voicesは永久固定値ではなく、Prototype後にPerformanceと音楽的必要性を検証して見直せる値とします。

## Voice Stealing

TrackのVoice上限へ到達した状態で新しいVoiceが必要になった場合、既存Voiceを選んで終了させます。

v0.1では、実装と挙動が予測しやすい以下のRuleを使用します。

優先順位:

1. すでにRelease段階に入っている最も古いVoice
2. それ以外の最も古いVoice

選ばれたVoiceは短いDe-click Fadeを経て終了し、新VoiceへSlotを渡します。

Amplitude測定による「最も静かなVoice」判定は、初期版では必須にしません。

理由:

- DeterministicでTestしやすい
- Audio Thread上で余計な分析処理を増やさない
- Release済みVoiceを先に整理できる

将来Synthや実機評価で必要になれば、EngineごとのSteal Priorityを追加できます。

## Choke Group

Sampler Engine StateはOptionalな`chokeGroup`を持てる構造とします。

概念:

```text
chokeGroup = null  // no choke
chokeGroup = 1
chokeGroup = 2
...
```

同じChoke Groupに属する新しいSampler Triggerが発生した場合、そのTriggerより前から存在する同GroupのSampler Voiceを終了します。

代表例:

```text
Closed Hat -> chokeGroup 1
Open Hat   -> chokeGroup 1
```

Open HatのTail再生中にClosed HatをTriggerすると、既存Open Hat Voiceを短いDe-click Fadeで停止してClosed Hatを開始します。

### ChokeのScope

Choke GroupはTrack内だけでなく、現在のProject Audio Graph上のSampler Voicesに対して共通に評価します。

これによりOpen HatとClosed Hatを別Trackへ置いてもChokeできます。

### Polyphonic TriggerとChoke

1つのSequencer Eventから複数Note Voiceを同時生成する場合、Choke判定は**Trigger Batch単位**で行います。

手順:

1. 新Trigger BatchのChoke Groupを確認
2. そのBatch以前から存在する同Group VoiceをChoke
3. Batch内の新しい複数Voiceを生成

同じChord / Trigger Batchから生成されたVoice同士は互いにChokeしません。

## Pattern Loop

同じPatternをLoopしている場合、既存仕様どおり長いNote / GateはPattern境界をまたいで継続できます。

Loop先で同じSampleや同じPitchが再Triggerされても、標準では新しいVoiceを追加します。

Voice上限を超えた場合のみVoice Stealing Ruleを適用します。

## Pattern切替

別Patternへ切り替える小節境界では、旧Pattern由来のVoiceをPlayback Modeに応じて扱います。

### Gate

旧Pattern由来のActive Gate VoiceへNote Offを送り、Releaseへ移行します。

### Loop

旧Pattern由来のActive Loop VoiceへNote Offを送り、Releaseへ移行します。

### One Shot

通常のNote Offでは止めず、原則としてSample EndまでTailを許可します。

これは既存のPattern仕様にある「One Shot Tailを可能な限り自然に残す」方針と一致します。

旧PatternのOne Shot VoiceはTrigger時のSampler Stateを保持しているため、新PatternのEngine Stateへ切り替わっても発音内容を変更しません。

新Patternで同じChoke GroupのTriggerが発生した場合は、旧PatternからCarryされたOne Shotも通常のChoke対象にできます。

## Transport Stop

Pattern切替とTransport Stopは区別します。

UserがTransportを明示的にStopした場合、Sequencer由来のSampler Voicesは停止対象とします。

- Gate / Loop: Note Off / Releaseへ移行
- One Shot: 短いDe-click Fadeを用いて停止可能

これにより「Stopしたのに長いOne Shotが数秒鳴り続ける」状態を避けます。

将来、Performance用途でTailを残すStop Modeが必要になればOptionとして追加できます。

Audition / Preview VoiceをTransport Stopと同時に止めるかは、Audition UI実装時に別途定義します。

## Missing Asset

Trigger時点で`sampleAssetId`がMissingの場合、新しいVoiceを生成しません。

既に正常に生成済みのVoiceは、その再生に必要なSource Resourceを安全に保持できる実装であれば、Project Metadata上のAsset状態変更だけを理由に途中停止させる必要はありません。

具体的なResource LifetimeはAudio Backend選定時に決定します。

## Runtime StateとPersistence

Voiceそのものは保存しません。

保存対象:

- playbackMode
- chokeGroup
- Sampler Parameters
- Sample Asset Reference
- Track / Step Data

保存しないもの:

- Active Voice一覧
- Current Playback Position
- Envelopeの現在Phase
- Voice Age
- Voice Stealing Queue

Project Load後は停止状態から再構築します。

## v0.1 Sampler Voice Scope

確定する初期Behavior:

- One Shotは通常のNote Offを無視してSample Endまで再生
- GateはNote OffでReleaseへ移行
- LoopはNote Off後もRelease中はLoop可能
- 同一Sample Retriggerは標準でOverlap
- 1 Sampler Trackあたり最大8 Voicesを暫定上限
- Voice StealingはRelease中の最古Voice、次に全体の最古Voice
- Choke GroupをSampler Track間で共有可能
- ChokeはTrigger Batch単位で評価
- Pattern切替ではGate / LoopをRelease、One Shot Tailは原則Carry
- VoiceはTrigger時点の有効Sampler Stateを保持
- Transport StopではSequencer由来Voiceを停止可能

## 今後の検討

この仕様を土台に、次は共通Voice Management / Audio Engine Schedulingとして以下を整理します。

- Synth Voiceとの共通Interface
- Note On / Note OffのSample-accurate Scheduling
- Audio CallbackとScheduler Queueの責務分離
- Global Polyphony Limit
- EngineごとのVoice Budget
- Choke / Steal / All Notes Offの優先順位
- Audio Buffer SizeとLatency
- Pattern切替EventをAudio Threadへ安全に渡す方法
- Parameter LockをVoice生成時に解決する境界
