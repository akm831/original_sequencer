# 設計上の決定事項と未決事項

このファイルには、明示的に見直さない限り維持する設計判断を記録します。

## 確定済みの判断

### 製品の位置づけ

本プロジェクトは単なるDrum Machine Cloneではなく、Groovebox型の音楽制作アプリとします。

### 基本操作

Main Sequencing Interactionは、分かりやすい16-step Surfaceを使用します。高度なControlはDetail Viewへ分離します。

### Track Model

TrackがEngineを選択します。初期EngineはSamplerとSynthです。Drum用とMelody用で無関係なSequencer Coreを別々に作りません。

### Step Model

Stepは拡張可能とし、Chord用の複数Noteを含むNote Eventを保持できるようにします。

### Note / Chord入力方式

Synth TrackおよびPitched Sampler TrackのStep Editorでは、同じ`notes[]`を編集する3つのInput Modeを採用します。

- NOTE: 単音を直接指定
- CHORD: Harmony EngineからChordを生成
- KEYBOARD: 任意のNote Combinationを直接入力

これらは別々のSequencer Event Typeにはしません。Audio Engineは最終的な`notes[]`を受け取ります。

CHORD Modeで生成したNotesをKEYBOARD Modeで後から調整できる設計を目指します。

### VelocityとAccent

VelocityとAccentは別々の音楽的概念として保持します。

### Musical Timing

TimingはMillisecondsだけでなくMusical Unitsで保持します。現在の設計目標は960 PPQNです。

### Step Resolution

v0.1ではMain Gridを1 Step = 16分音符に固定します。4/4では16 Steps = 1小節です。

Step Resolution変更は将来機能とし、初期版では16-Step Grooveboxとしての分かりやすさを優先します。

### Note Length

Note LengthはStep数そのものではなくMusical Tickで保持します。960 PPQNでは16分音符1 Step = 240 ticksです。

UIではTickを直接入力させず、25% / 50% / 75% / 100% / 2 Steps / 4 Steps等の音楽的な単位で操作します。

v0.1ではChord内の複数Noteは同じLengthを共有します。

### Tie

Tieは独立したSequencer Eventとして保存せず、直前のNote / ChordのLengthを延長する編集操作として扱います。

これによりSynth、Sampler Gate、将来のMIDIで同じDuration Modelを使用できます。

### Legato

LegatoはTieと分離します。

Tieは同じEventの継続、Legatoは次のNoteへ移行するときのEnvelope RetriggerやGlide等の発音Behaviorです。

初期版では主にMono Synth向けの将来機能として`legato`拡張点を残します。

### SwingとMicro Timing

Swingは規則的なGroove Timing、Micro TimingはStep単位のOffsetとして分離します。

### Track Length

Trackごとの独立LengthをCore Capabilityとします。初期範囲は1–16 Stepsです。

### Pattern切替

演奏中に別Patternを選択した場合、標準では即時切替せず、次のProject小節境界で切り替えます。

v0.1では4/4・16分Stepを基準として、16 Global Steps = 1小節をPattern Launch Quantizationとします。

各Trackが15 Stepsや7 StepsでLoopしていても、Pattern切替基準はTrack Loop終端ではなくProject Transportの小節境界です。

### Pattern切替時のPhase

新しいPatternへ切り替えるときは、新Pattern内の全TrackをStep 1から開始します。旧PatternのTrack Phaseは引き継ぎません。

### Pattern BPM

BPMはPatternごとではなくProject Globalとします。Tempo Changeが将来必要になった場合はArrangement Layerで扱う方針を優先します。

### Pattern Copy

Pattern DuplicateはStep Dataだけでなく、Track Length、Engine Type、Engine State、Mixer Stateを含むPattern全体を複製します。

### Pattern境界の長音

同じPatternをLoopしている場合、Note LengthはPattern境界をまたいで継続可能とします。

別Patternへ切り替える境界では旧Pattern由来のGate保持中NoteへNote Offを送り、Synth ReleaseやOne Shot Sample、将来のEffect Tailなどの自然なAudio Tailは原則として許容します。

### Pattern拡張

v0.1のMain Surfaceは16 Stepsを維持します。将来32 / 64 Stepへ拡張する場合は、16 Steps単位のPage方式を基本候補とします。

Pattern ChainとSong / Arrangement ModeはPattern Sequencerより上位Layerへ追加します。

### Sampler編集

Sample Start / End、Reverse、Root Note等は原則として非破壊編集にします。

### Sampler Voice Management

Sampler Triggerは独立Voiceを生成し、同一SampleのRetriggerは標準でOverlapを許可します。

Sampler v0.1は1 Trackあたり最大8 Voicesを暫定上限とし、Voice StealingはRelease中の最古Voice、その後に最古のActive Voiceを選びます。

Choke GroupはTrack間で共有可能とし、Pattern切替ではGate / LoopをReleaseへ移行し、One Shot Tailは原則Carryします。

VoiceはTrigger時点のResolved Sampler Stateを保持します。

### Synthの方向性

最初のBuilt-in Synthは巨大なThird-party Instrumentを組み込むのではなく、コンパクトなSubtractive Synthesizerとします。

Synth v0.1は以下を基本構成とします。

- 2 Oscillators + Noise
- Sine / Triangle / Saw / Square
- LP / HP Filter
- Amp ADSR
- Filter ADSR
- 1 LFO
- Mono / Poly
- Mono時のGlide

Sub Oscillator、FM、Wavetable、Modulation Matrix等は初期版では追加しません。

### Synth LFO

LFOはv0.1からFree / Tempo Syncの両方を扱える構造とします。

初期DestinationはPitch、Filter Cutoff、Amplitudeとし、1 LFOにつき1 Destinationを基本とします。

### Synth UI

スマートフォンでの操作性を優先し、Synth ParameterをMain Step Gridへ大量に表示しません。

Synth Editorは以下の少数Pageへ分離する方針です。

- OSC
- FILTER
- ENV
- MOD / VOICE

### Synth Parameter Lock

Parameter LockはまずContinuous Parameterを優先します。

例:

- Filter Cutoff
- Resonance
- Tune / Detune
- OSC Level
- Envelope Time
- LFO Rate / Amount

Oscillator Wave、Filter Type、Voice Mode等のDiscrete Parameter切替は、ClickやVoice Stateへの影響を検証してから拡張します。

### Chord生成

将来のChord / Harmony機能はAudio Engineより上位に置き、通常のNoteを生成します。特殊なSynth PresetだけでChord機能を実現しません。

### Harmony初期Chord方針

初期Harmony機能は、最大4音程度のChordを簡単に入力・再生できることを優先します。

Triadおよび7th系Chordを中心にしつつ、内部データモデルは4音固定にしません。

将来的には9th、11th、13th、add系、Altered Tension等のExtended Chordを扱えるようにし、Harmony Engineは任意数のGenerated Notesを扱える設計とします。

### Harmony UI

CHORD ModeではRoot、Chord Type、Octave、Inversionを基本Controlとし、生成NoteをPreview表示します。

可能であればChord選択時にAuditionできるようにし、理論知識だけでなく耳でChordを選べる設計とします。

高度なTension、Alteration、VoicingはDetail Viewへ分離します。

### Audio Engine Scheduling

Sequencer / TransportとRealtime Audio Callbackを分離し、SchedulerがLookaheadでTimestamp付きAudio Commandを準備します。

Audio CallbackはBuffer内のEventを可能な限りSample Offset位置で実行し、UI / Project Model / File I/Oへ直接依存しません。

Parameter LockはAudio Thread外でEvent-localなResolved Stateへ解決します。

Common Voice ContractはLifecycleだけを共有し、Sampler / Synth固有DSP Stateは各Engine内部に保持します。

### Audio Buffer / Latency

Audio Logicを特定のBuffer Sizeへ固定しません。

v0.1の性能目標は128 Frames程度をPreferred、256 Frames程度をStable Fallbackとし、512 Frames以上でもCompatibility動作できる構造を維持します。

Sequencer Lookaheadは約50 msを初期候補としますが、Live InputはそのLookaheadを待たないLow-latency Pathを持ちます。

Scheduler QueueはBoundedとし、Overflow時もAudio ThreadをBlockせず、Stop / Panic系Commandを失いにくいFail-safeを持ちます。

### Global Polyphony / Performance Budget

Track Voice LimitとGlobal Safety Budgetを分離します。

Sampler / Synth Polyは各Track最大8 Voicesを暫定上限とし、Engine構造は最大64 Active Voiceを表現可能にします。

ただし64 Voicesを全Device / 全Patchで保証するとはしません。

Framework / Backend比較では32 Concurrent VoicesをBaseline Performance Target候補、64 Light-to-Moderate VoicesをStretch Targetとします。

Voice CountだけでなくCallback処理時間を主要性能指標とし、Reference ProjectではCallback Load約50%以下をNormal Targetとします。

Global Budget到達時はRelease中の古いVoiceを優先して整理し、通常再生でGlobal Stealが頻発する状態は正常とみなしません。

Dynamic Quality Scalingはv0.1必須にはせず、安定性、Timing、予測可能なVoice制限、Buffer Fallbackを優先します。

### Factory Content

同梱Sample / Presetは再配布可能なLicenseが明確なものだけを使用します。

## 暫定的な判断

以下は現時点での方針ですが、Prototype後に見直す可能性があります。

- 初期Groovebox目標は約8 Tracks
- 初期約8 Patterns
- Factory Samplesは約70–100
- Synth Presetsは約30
- v0.1はMaster Swing、Track Swingは後で追加
- Independent Track Lengthを確認した後にTrack Rateを追加
- Sampler Polyphonyは1 Sampler Trackあたり最大8 Voiceを暫定上限とする
- Synth Polyphonyは1 Synth Trackあたり最大8 Voiceを暫定目標とする
- KEYBOARD Modeは1〜2 Octave程度の横スクロール可能なTouch Keyboardを想定
- Pattern UIはCurrent / Queued / Idleを区別する
- 128 Frames程度をPreferred Audio Buffer Targetとする
- Sequencer Lookaheadは約50 msを初期候補とする
- 32 Concurrent VoicesをBaseline Performance Target候補とする
- 64 Light-to-Moderate VoicesをStretch Performance Targetとする

## 未決事項

### Platform / Framework

未決定です。

これまでの候補:

- 必要に応じNative Audio Integrationを組み合わせたFlutter
- Web Technology + Native / Mobile Wrapper
- 高性能DSP向けNative / JUCE / C++ Audio Layer

単なる実装の手軽さだけでなく、製品要件から決定します。

### Audio Engine

未決事項:

- Fully Shared EngineかPlatform-native Layerか
- Global Voice LimitのDevice / Backend別具体値
- Resampling / Interpolation Quality
- Filter Implementation
- Effects Architecture
- Sample Streaming / Caching Strategy
- Synth Mono Legato / Retrigger Semantics
- Continuous Automation / Live Parameter補間

### Project Format

要決定:

- Serialization Format
- Explicit Schema Versioning
- Asset Reference / Portable Project Bundle
- Missing Sample時のBehavior
- Version間Migration

### Step Semantics

詳細未定:

- ProbabilityとConditional Triggerの評価関係
- Repeat / Ratchetの評価順
- Parameter LockのLifetime / Interpolation
- Polyphonic NoteへのParameter Lock
- Sample LockとSampler Parameter Lockの関係
- CHORDで生成したNotesをKEYBOARDで編集した場合のHarmony Metadata保持Rule
- Polyphonic Chordに対する将来のLegato Semantics

### Pattern Behavior

詳細未定:

- 32 / 64 Step化した際のTrack LengthとPageの関係
- Pattern Chainの編集UI
- Song / Arrangement Modeの具体的Data Model
- Pattern切替を1小節以外にも設定可能にするか

### Sampler

詳細未定:

- 専用Loop Start / EndやLoop Crossfade等を含む高度なLoop Behavior
- Start / EndのUnitとPrecision
- One Shot Envelopeの詳細
- Slice Representation
- Sample Caching / Streaming Strategy
- PlatformごとのSupported File Formats

### Synth

詳細未定:

- 8 Voice暫定値の実機性能検証
- Engine固有Voice Stealing Overrideが必要か
- Oscillator Anti-aliasing Strategy
- Filter Algorithm / Character
- LFOのTriplet / Dotted対応時期
- Preset Formatの具体的Schema
- VelocityをFilter等へどこまでRoutingするか
- Mono Legato時のEnvelope Retrigger Rule

### Harmony

詳細未定:

- Chord MetadataとGenerated Notesのどちらを永続化するか
- 初期UIで表示するChord Vocabularyの具体範囲
- Extended Chordを`Chord Type`だけで表すか、Base Chord + Extension / Alterationで表すか
- Scale Model
- Chord Suggestion Behavior
- Voicing Constraints
- Tension Chord時の省略音（Omit）Rule
- Synth Voice Stealingを考慮した実用上の最大Chord構成音数

## Decision Log形式

重要な技術・製品判断を行った場合、以下の形式で追記します。

```text
YYYY-MM-DD — 判断タイトル
決定:
理由:
検討した代替案:
影響:
```

これにより、後のCodex Sessionで背景を失ったまま確定済みArchitectureを再判断することを防ぎます。

## Decision Log

```text
2026-09-12 — Synth v0.1の基本構成
決定:
2 OSC + Noise、LP/HP Filter、Amp/Filter ADSR、1 LFO、Mono/Poly、GlideをSynth v0.1の基本構成とする。
LFOはFree / Tempo Syncを持つ。1 Synth Trackあたり8 Voiceを暫定目標とする。

理由:
GrooveboxとしてBass、Lead、Pluck、Pad、Chordを十分作れる一方、スマートフォンUIとDSP実装を過度に複雑化しないため。
Parameter Lockとの相性が良いContinuous Parameterを中心に構成できるため。

検討した代替案:
Sub Oscillator、FM、Wavetable、複数LFO、Modulation Matrixを初期版から搭載する案。

影響:
初期Synthは機能数よりSequencer連携と操作性を優先する。
高度なSynthesis機能は後からSynth Engine内部へ追加する。

2026-09-12 — HarmonyのChord音数とTension対応
決定:
初期Harmony UIでは最大4音程度のChordを優先する。
内部モデルは4音固定とせず、将来9th / 11th / 13th等のExtended ChordやAltered Tensionへ拡張可能にする。

理由:
初期UIを簡潔に保ちつつ、将来のJazz、Neo Soul、House、R&B等で必要になるHarmony表現を制限しないため。

検討した代替案:
初期版から7音程度のExtended Chord編集を全面的に搭載する案、Harmony Engine自体を4音固定にする案。

影響:
初期実装はTriad / 7th系を中心にする。
ChordEventは将来Extension / Alteration / Voicingを追加できる構造を維持する。

2026-09-12 — StepのNote / Chord入力方式
決定:
Step EditorにNOTE / CHORD / KEYBOARDの3 Input Modeを設け、すべて同じ`notes[]`を編集する。
CHORD ModeはHarmony EngineからNotesを生成し、KEYBOARD Modeでは任意のNotesを直接編集する。

理由:
単音入力、簡単なChord Name入力、自由なVoicing編集を1つのSequencer Modelで共存させるため。
将来Synth、Pitched Sampler、MIDIで同じNote Eventを利用できるため。

検討した代替案:
Chord専用Step Typeを作る案、Synth内部でChordを発音する案、常にPiano Rollを使用する案。

影響:
Audio EngineはChord Nameではなく最終的な`notes[]`を受け取る。
Harmony Metadataは入力・再編集支援のための補助情報として扱う。

2026-09-12 — Note Length / Tie / Legato / Step Resolution
決定:
v0.1は1 Step = 16分音符で固定する。
Note LengthはMusical Tickで保持する。
Tieは独立Eventにせず、直前EventのLengthを延長する編集操作とする。
LegatoはTieと分離し、主にMono Synthの発音Behaviorとして扱う。

理由:
TR系16-Stepの即時性を維持しながら、Bass、Melody、Chordで短音から複数Stepにまたがる長音まで共通Modelで表現するため。
Tie専用EventをAudio Engineへ持ち込まず、Sampler / Synth / MIDIでDuration Semanticsを共通化するため。

検討した代替案:
StepごとにTie Eventを保存する案、初期版から複数Resolutionを自由に切り替える案、LengthをStep数だけで保存する案。

影響:
960 PPQNでは16分音符1 Step = 240 ticksとなる。
UIはTickではなくStep比率やStep数でLengthを表示する。
Resolution変更、Polyphonic Legatoは将来詳細化する。

2026-09-13 — Pattern構造と切替
決定:
v0.1のPattern切替は次のProject小節境界でQuantizeする。
各Trackの独立Lengthとは無関係に、Project Transportの16 Global Stepsを1小節のLaunch境界とする。
新Patternへ切り替える際は全TrackをStep 1から開始する。
BPMはProject Globalとする。
Pattern DuplicateはSound / Mixer Stateを含むPattern全体を複製する。

理由:
Polymetric Trackを許容しながらも、Pattern切替のタイミングを演奏者が予測しやすくするため。
PatternごとのTempoやTrack Phase継承を初期版から入れると、Live操作とSchedulingが複雑になるため。

検討した代替案:
Tap直後に即時Pattern切替する案、各TrackのLoop終端を待つ案、PatternごとにBPMを持つ案、Track PhaseをPattern間で継承する案。

影響:
UIはCurrent / Queued / Idleを区別する。
同一Pattern Loopでは長いNoteが境界をまたげる。
別Pattern切替時は旧PatternのGate Noteを終了しつつ、Release / One Shot / Effect Tailは可能な限り自然に残す。
将来32 / 64 Step化は16-Step Page方式を優先し、Pattern Chain / Song Modeは上位Layerへ追加する。

2026-09-13 — Sampler Voice Management
決定:
同一Sample Retriggerは標準でOverlapを許可し、Sampler Trackは最大8 Voicesを暫定上限とする。
Voice StealingはRelease中の最古Voice、その後に最古Voiceを選ぶ。
Choke GroupはTrack間で共有可能とし、Pattern切替時はGate / LoopをRelease、One Shot Tailは原則Carryする。

理由:
Drum / FX TailやPitched Sampler Chordを自然に扱いながら、Voice数を予測可能に制御するため。

検討した代替案:
同一SampleをRetriggerするたび前Voiceを停止する案、Track内だけのChoke、無制限Polyphony。

影響:
Sampler VoiceはTrigger時点のResolved Stateを保持する。
Audio Engine側に明示的なVoice Lifecycle / Choke / Steal処理が必要になる。

2026-09-13 — Audio Engine Scheduling
決定:
Sequencer / SchedulerとRealtime Audio Callbackを分離し、Timestamp付きAudio CommandをQueue経由で渡す。
Buffer内Eventは可能な限りSample Offset位置で実行する。
Parameter LockはAudio Thread外でResolved Trigger Stateへ解決する。

理由:
UI負荷やProject Model探索からAudio Threadを隔離し、Step TimingをBuffer境界へ量子化せず安定再生するため。

検討した代替案:
Audio Callbackが直接Project Modelを走査する案、EventをBuffer先頭へ丸める案。

影響:
Realtime-safe QueueとLookahead Schedulerが必要になる。
Framework / Backend比較ではSample-accurate Schedulingを実現できるかが重要評価項目になる。

2026-09-13 — Audio Buffer / Latency
決定:
128 Frames程度をPreferred Target、256 Frames程度をStable Fallbackとする。
Sequencer Lookaheadは約50 msを初期候補とするが、Live InputはLookaheadを待たないLow-latency Pathを使用する。

理由:
低LatencyとMobile端末での安定性を両立し、Sequencerの先読みがLive演奏Latencyを直接増やさないようにするため。

検討した代替案:
64 Frames以下をv0.1必須にする案、すべてのInputを同じLookahead経路へ通す案、Buffer Sizeを固定する案。

影響:
Audio LogicはVariable Buffer Sizeへ対応する。
Device Restart時にはAudio Frame OriginとPending Queueを再構築する。

2026-09-13 — Global Polyphony / Performance Budget
決定:
Track Voice LimitとGlobal Safety Budgetを分離する。
Engine構造は最大64 Active Voiceを表現可能にし、32 Concurrent VoicesをBaseline Performance Target候補、64 Light-to-Moderate VoicesをStretch Targetとする。
Reference ProjectではCallback Load約50%以下をNormal Targetとし、継続的な70〜80%以上はHeadroom不足として評価する。

理由:
Voice数だけではSamplerとSynthの実CPU Cost差を表せず、Realtime Audioの安全性はCallback Deadlineへの余裕で評価する必要があるため。
Mobile向けGrooveboxとして3〜4音Chord、Release Tail、複数Trackを現実的に扱える共通Benchmarkを持つため。

検討した代替案:
Global Voice数だけで性能を判断する案、全Deviceで64 Heavy Synth Voicesを保証する案、負荷に応じて演奏中に音質を自動変更する案。

影響:
Framework / Backend比較ではSampler-heavy、Synth-heavy、Mixed、BurstのReference Benchmarkを共通で使用する。
Performance ProfileはDevice / Backend側で選びProjectには保存しない。
Global Budget到達はSafety Fallbackとし、通常再生で頻発する場合は性能要件またはProfileを見直す。
```


## 2026-10-02 — P3 Sample Offset検証の最小発音経路

Technology PrototypeのP3をHost上で検証できるよう、Common CoreのTrigger Commandは50 ms / 220 Hz / 最大Amplitude 0.08のMonophonic検証用Burstを開始する。Commandの時刻まで先行区間をRenderしてからVoiceを開始することで、DiagnosticsのOffset記録と実際の出力Sampleを一致させる。

Flutterの検証ボタンはScheduled Command入力であり、製品版Live Padではない。Live PadはLookahead末尾に入れない既存方針を維持する。Poly Voice / Voice Steal / 製品版Oscillatorの仕様はこの検証音から確定しない。

Android AdapterのControl側でCommand投入とCore Lifecycleを直列化する。Audio CallbackへMutex、UI、allocationを持ち込まない。旧Streamの遅れたError Callbackによって新Streamを再OpenしないようStream identityを確認する。

## 2026-10-03 — P4 Transport / Lookaheadの最小実装

共有C++のControl-side SchedulerをFlutter候補へ先に接続する。1 Track / 16 Steps、960 PPQN、240 ticks/step、60〜240 BPM。Android Native Threadが5 ms間隔で約50 ms先までCommandを投入し、Audio CallbackはTimestamp実行のみ。UIのTimerで音を予約しない。

Stop / Restartは世代番号で予約済みEventを無効化し、Queueを実行中にResetしない。停止時の50 ms検証Burst Tailは自然終了を許可する。Live Padは未実装であり、P3テスト入力はTransport停止時のみ有効とする。BPM / Step編集は未予約Eventへ適用し、既にQueueへ入れたEventを書き換えない。Schedulerが遅れた場合は過去のStepをSkipし、Overflowの場合はTransportを停止する。

Pattern保存、複数Track、Sampler / Synth、JUCE P4はこのCheckpointの対象外。Technology選定は引き続き実機比較で判断する。

## 2026-10-03 — P4ステップ表示遅延の修正

実機で発音が正常だがステップ強調が遅いとの報告を受けた。50 ms周期で全画面診断を更新していたため、再生位置の観測に最大約50 msの待ちが生じていた。再生中のみFlutterのフレーム同期TickerでNativeの実行済み位置を読み、値の変化時だけ再描画する。ステップボタンのStyle AnimationはDuration.zeroへ変更する。診断は200 ms周期へ分離する。音のSchedulerや発音時刻は変更しない。

表示はレンダー済み位置の観測であり、音声出力装置のPresentation Timestampとの厳密な同期ではない。画面描画と音声DeviceのLatencyは残るため、実機での改善量は別途確認する。

## 2026-10-03 — P4操作画面と診断の分離

メインは再生／停止、BPM、16 Stepsに絞り、音声診断とP3単発テストはAppBarから開く別画面へ移す。Gridは縦画面で利用可能な高さへ収め、短い横画面では48 logical pixels以上のTouch Targetを維持してGrid部分のみScrollする。Native Transportを保持するScreen Stateと表示Widgetを分離し、Presentationから音のSchedulingを行わない。

PrototypeのApp離脱時はTransportを停止し、復帰時に自動再生しない。Audio Device Stream自体は維持する。このCheckpointでBackground Playback / Audio Focusの製品仕様は確定しない。別画面へのApp内移動では再生を継続する。

ユーザーのM4 Macでは現在のARM64 APKとARM64 AVDを使う。EmulatorはUI / 操作確認へ利用し、実音声Latency / Route Change / 継続負荷は実機評価を維持する。技術選定はJUCE同等経路の比較を残しており、このUI整理でFlutter採用を確定しない。

## 2026-10-03 — JUCE P4比較と自動APK経路

JUCEにも共有PrototypeSequencerを接続し、Android / Flutterと同じ960 PPQN、16 Steps、60〜240 BPM、約50 ms Lookahead、50 ms / 220 Hz検証Voiceを使う。JUCEのControl Threadが5 ms周期で予約し、Message Threadは観測と操作のみ、Audio CallbackはPlanar出力のSample Offset実行のみ。SuspendでTransport停止、Resumeは停止を維持する。

JUCE Androidは既存のProjucer Export経路を自動化する。JUCE 9.0.2をCommit `72782788ce18c2d4d760b28e0921d6ffc6431102`で検証し、SDK 36 / min 24、NDK 28.2.13676358、JDK 17を固定する。Exporterが固定出力するNDK 28.1とJava 8 toolchainだけ生成後Scriptで調整し、Java 8 bytecode互換性は保つ。生成Gradle Projectは保存元にせず.jucerとScriptを正本とする。

Application IDはcom.originalsequencer.juceprototypeとしてFlutterと同時Install可能にする。P4 Build成功だけではTechnologyを決定せず、実機比較と既存Must Gateを維持する。

## 2026-10-03 — Android優先の開発順とPlayable Groovebox

ユーザーの明示指示により、Flutter / JUCEの交互比較を開発の前提から外し、実機で動作確認済みのFlutter + C++ / OboeをAndroidの当面の実装経路とする。JUCE比較Workflowは手動実行に変更。Web / iOS / Desktop互換性と最終Technology選定は後続段階で検証し、iOS Must SmokeをAndroid機能開発の開始条件にはしない。CoreへのAndroid API持ち込み禁止は維持する。

次のVertical Sliceは4 Tracks × 4 Patterns × 16 Steps。Trackごとに独立した簡易合成音源（Kick / Snare / Hat / Bass）を同時発音し、Volume / Mute / Accentを編集できる。製品仕様の約8 Tracks、Engine選択、Note / Gate / Samplerは後続。これは最終SynthやVoice Budgetの実装ではない。長押しはこの段階ではAccent切り替えとし、製品Step Editorへ後で置き換える。

Pattern選択は再生中は次の未予約の小節先頭に適用し、最新の予約要求を採用する。約50 msのLookahead内の予約済みEventは書き換えない。表示はAudio Callbackで実行済みのPattern / Stepを観測する。Stopは予約Patternを取り消し、最後に実行したPatternを保持し、次のPlayはStep 1から開始する。VoiceはTrackごとにMonophonic / Retrigger、Tailは最大200 ms。無音StepにもMarkerを置く。

Android内部領域へVersion 1 / kind android-groovebox / PPQN 960のJSONを保存する。ModelとPlatform I/Oを分離し、BPM、編集Pattern、全TrackのEngine ID / Step / Accent / Volume / Muteを保存。Edit後500 ms debounce、手動保存、App離脱時保存。Kotlinの単一I/O ExecutorとAtomicFileで書き込み順と失敗時の旧データを保つ。未知Schema / 不正データ / 読み込み失敗は上書きを停止する。再起動時に復元し、自動再生しない。外部Import / Exportと製品Portable Project Formatは後続。

## 2026-10-04 — 音源を808系Percussion / 303系Bassへ優先拡張

ユーザー指示でSampler Import / Parameter Lockより、808系Percussionと303系Bassの特徴を持つ合成音源を優先する。製品Synth全仕様を一度に実装せず、4 Track構成のままKick / Snare / Closed・Open Hat / Mono Acid Bassを演奏できるSliceにする。回路の厳密なEmulationや完全なTR-808 / TB-303再現を保証しない。Factory Sampleを使わないため外部音源素材に依存しない。

共通C++の固定容量AnalogVoiceをAudioCoreから利用。KickはPitch Sweepと減衰Sine、Snareは2 Body TonesとHigh-pass Noise、Hatは6 Metallic SquaresとHigh-pass成分。Closed / Openは同じVoiceを共有してChoke。Pitch / Tone / DecayをBankごとに保存。BassはPolyBLEP Saw / Square、飽和Feedback付き3段LP、Filter Envelope、Accent、65 ms相当のExponential Glide。2 Integration Substepsは簡易安定化であり、完全Oversampling / 回路モデルとは区別する。

Bass NoteはMIDI 24〜84、Step長240 ticks、通常GateはStepの65%、Outgoing Slideが有効かつ直後の発音がある場合は105%でつなぐ。Incoming Slideは同じPatternの連続した有効Stepだけで発生し、休符 / Pattern変更 / Stop / 世代変更では切れる。Slide中はOscillator / Amp / Filter Envelopeを再Triggerしない。Accentは音量とFilter Envelope深さを増やす。Gate以降は6 ms相当Release、最長Voice寿命はPercussionの約980 ms。予約済み音色変更は従来同様Lookahead内で書き換えず、新しいEventへ適用。

Step長押しを詳細Editorへ変更。発音とAccent、Bass Note / Outgoing Slide、Hat Openを編集。Track音色EditorはScroll可能にしてMain Gridを簡潔に保つ。JSON Schema 2へSound State / notes[16] / flagsを追加し、Schema 1のPattern / Mask / Accent / Volume / Mute / BPMをそのまま保持してDefaultsでMigration。未知Version / 不正Stateの上書き停止は維持。

音色設計の操作項目はRolandの[TB-303取扱説明書](https://cdn.roland.com/assets/media/pdf/TB-303_OM.pdf)を参考にした。音声処理コードはこのProject独自の特徴合成で、Rolandの回路Emulationではない。

## 2026-10-04 — 音源拡張後の安定化とAndroid Audio Focus

ユーザーの継続作業指示を受け、再生中のTrack更新を18個のFFI Callから検証済みSnapshotの単一Callへ変更する。Sound / Notes / Flags / Mask / Level / MuteをControl Mutex内で一括Commitし、不正入力は一部だけ適用しない。Dart側の一時AllocationはUI Threadのみで、Audio Callbackは固定容量を維持。C ABI構造体112 bytes、APK Exported Symbolも検査する。

新Analog VoiceのStop / 世代変更は最大20 msのFadeで終了。通常のRetriggerは直前Sampleから約2 msで新Voiceへ移行し、急な波形Resetを緩和。P3 Legacy Burstは従来の50 ms Tailを保持。Playは編集Bankを明示してStep 1から開始する。

AndroidではPlay直前にMEDIA / MUSICのAudio Focusを要求し、拒否時は再生しない。LOSS / LOSS_TRANSIENT / CAN_DUCKは停止し、GAIN時に自動再生しない。Stop / App離脱 / DisposeでFocusを返す。非同期Focus応答の世代を追跡し、Stop後の遅いGrantが勝手に再生を開始しないようにする。P3 Testも短時間だけFocusを取得。API 24〜25はLegacy AudioManager、26以降はAudioFocusRequestを利用。Oboe Usage / ContentTypeをFocus Attributesに合わせる。Background Playbackは追加しない。

[Android公式Audio Focus文書](https://developer.android.com/media/optimize/audio-focus)を参照。実際の他アプリ割り込み / 通話 / Bluetooth経路はBuildやHost testのみでは保証できないため実機確認を残す。

## 2026-10-04 — テストAPK署名固定とProject Backup

従来CIはRunnerごとにAndroid default debug keystoreが変わり得るため、更新Installの署名互換性を保証できなかった。以後はRepository内の公開TEST ONLY PKCS12鍵でDebugを署名し、APKの証明書SHA-256をCIで検査・build-info.jsonへ記録する。この鍵は秘密ではなく製品Releaseに使わない。過去のAPKとは署名が異なる可能性があるため、初回移行前にProjectを退避する。過去APKのprivate signing keyは現在の成果物から復元できない。

Android Storage Access Frameworkのファイル選択で現在のJSONを書き出し／読み込みできる。読み込みはサイズ上限・Schema検査・ユーザーReview・既存内部保存の別名Backupをすべて通してから反映する。Cancel / Parse Error / Backup Errorでは置き換えない。読めない旧保存のExportはUI Defaultではなく元のRaw JSONを使う。Sample Asset未導入のため今回はMetadata単体、製品Bundle Formatは後続。
