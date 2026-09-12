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
- Synth Polyphonyは1 Synth Trackあたり最大8 Voiceを暫定目標とする
- KEYBOARD Modeは1〜2 Octave程度の横スクロール可能なTouch Keyboardを想定
- Pattern UIはCurrent / Queued / Idleを区別する

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
- Sample-accurate Scheduling Strategy
- Global Maximum Polyphony
- Voice Stealing
- Resampling / Interpolation Quality
- Filter Implementation
- Effects Architecture

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
- 新しいTriggerが既存の長いNoteと重なった場合のVoice Rule
- Polyphonic Chordに対する将来のLegato Semantics

### Pattern Behavior

詳細未定:

- 32 / 64 Step化した際のTrack LengthとPageの関係
- Pattern Chainの編集UI
- Song / Arrangement Modeの具体的Data Model
- Pattern切替を1小節以外にも設定可能にするか
- Long NoteとLoop先の同Pitch Triggerが重なる場合のVoice Rule
- One Shot / FX TailをPattern切替時にどこまでCarryするかの詳細

### Sampler

詳細未定:

- Exact Loop Behavior
- Start / EndのUnitとPrecision
- One Shot時のEnvelope Behavior
- Slice Representation
- Sample Caching Strategy
- PlatformごとのSupported File Formats

### Synth

詳細未定:

- 8 Voice暫定値の実機性能検証
- Voice Stealing Rule
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
```
