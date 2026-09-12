# Sequencer仕様

## 基本階層

```text
Project
└─ Patterns[]
   └─ Pattern
      └─ Tracks[]
         └─ Track
            └─ Steps[]
               └─ Step
```

SequencerはSampler TrackとSynth Trackで共通利用します。

## Project Timing

初期目標:

```text
bpm
ppqn = 960
masterSwing
```

Swingは固定ミリ秒ではなく、音楽的な比率として表現します。

推奨UI表現:

- 50% = Straight
- それより大きい値で対象Subdivisionを遅らせる

## Step Resolution

v0.1ではMain GridのResolutionを固定します。

- 1 Step = 16分音符
- 4/4拍子では16 Steps = 1小節

960 PPQNの場合:

```text
Quarter Note = 960 ticks
8th Note     = 480 ticks
16th Note    = 240 ticks
32nd Note    = 120 ticks
```

初期版ではResolutionをUser設定にしません。まず16-Step Grooveboxとしての操作性を優先します。

将来はTrackまたはPattern単位で以下のResolutionを検討できます。

- 1/8
- 1/16
- 1/32
- Triplet系

ただしTrack Rateと役割が重なるため、両者のUIと内部意味は実装前に再検証します。

## Track

概念データ:

```text
Track
├─ id
├─ name
├─ engineType
├─ engineState
├─ length
├─ rate
├─ swing
├─ volume
├─ pan
├─ mute
├─ solo
└─ steps[]
```

### Track Length

初期実装目標:

- 各Track独立で1–16 Steps

例:

```text
Kick  16
Snare 16
Hat   15
Perc   7
```

これにより意図的にPolymetricで変化するPatternを作れるようにします。

### Track Rate

将来候補:

- 1/4x
- 1/2x
- 1x
- 2x
- 4x

最初のPlayable Milestoneでは必須としません。

## Step

Stepは単なるON/OFF Booleanではなく、拡張可能なMusical Event Containerとして扱います。

```text
Step
├─ enabled
├─ notes[]
├─ velocity
├─ lengthTicks
├─ accent
├─ legato
├─ microTimingTicks
├─ probability
├─ condition
├─ repeat
├─ sampleOverride
└─ parameterLocks
```

v0.1で全FieldをUIに出す必要はありません。

`lengthTicks`は固定Step数ではなくMusical Tickで保持します。これにより短いGateから複数Stepにまたがる長音まで同じModelで扱えます。

## Notes

Chordを表現できるよう、単一NoteではなくCollectionを使用します。

```text
Bass Note:
notes = [C2]

Chord:
notes = [C3, E3, G3]
```

この表現はBuilt-in Synth、Pitched Sampler、将来のMIDI Engineのいずれでも利用できるようにします。

`notes[]`は入力方法に依存しません。NOTE、CHORD、KEYBOARDのどのUIから入力しても、再生時には通常のNote Eventとして扱います。

## Step Note Editor

Synth TrackまたはPitched Sampler Trackでは、StepをLong PressしてNote Editorを開けるようにします。

初期設計では、以下の3つの入力Modeを同じEditor内で切り替えます。

```text
[ NOTE ] [ CHORD ] [ KEYBOARD ]
```

このModeは別々のSequencer機能ではなく、同じ`notes[]`を編集するための入力方法です。

### NOTE Mode

単音を素早く指定するModeです。

基本Control:

- Note Name: C〜B
- Sharp / Flatを含む半音
- Octave
- Velocity
- Length
- Accent

概念UI:

```text
NOTE

Pitch    C#
Octave    3

Velocity 110
Length    75%
Accent    OFF
```

Bass、Lead、単音Melody等ではこのModeを基本とします。

### CHORD Mode

Harmony Engineを使用してChord Nameから複数Noteを生成するModeです。

初期Control:

- Root
- Chord Type
- Octave
- Inversion
- 生成されるNoteのPreview
- Velocity
- Length

例:

```text
CHORD

Root      C
Type      m7
Inversion Root
Octave    3

Notes: C3 Eb3 G3 Bb3
```

初期Harmony UIでは最大4音程度を中心に扱いますが、`notes[]`自体は4音固定にしません。

将来は9th / 11th / 13th等のExtended Chord、Tension、Voicingにも同じCHORD Modeを拡張します。

### KEYBOARD Mode

画面上のPiano Keyboardを使用し、任意のNote Combinationを直接入力するModeです。

用途:

- Chord Nameでは表現しにくいVoicing
- 任意のInterval
- Chord生成後の手動調整
- 音を聴きながらの入力

初期UIでは1〜2 Octave程度の横スクロール可能なKeyboardを想定します。

複数Keyを選択できるようにし、選択Noteを`notes[]`へ格納します。

スマートフォンでは鍵盤を常時Main Gridへ表示せず、Step Editor内の専用Viewとして表示します。

## Note入力とChord Metadata

再生に必要な最終情報は常に`notes[]`です。

将来、CHORD Modeで入力したChordを後からChord Nameのまま再編集できるよう、以下のような補助MetadataをStepへ追加できる構造を許容します。

```text
harmonySource
├─ root
├─ chordType
├─ extensions[]
├─ alterations[]
├─ inversion
└─ voicing
```

ただしAudio EngineはこのMetadataへ依存せず、生成済みの`notes[]`を受け取る設計とします。

KEYBOARD Mode等で生成Noteを個別編集した場合にHarmony Metadataを保持・解除するRuleは、Harmony機能実装時に決定します。

## VelocityとAccent

VelocityとAccentは別概念です。

### Velocity

内部目標範囲:

- 1–127

将来のMIDI互換性も考慮します。VelocityはEventの強さを表します。

### Accent

Accentは独立した強調Flag / Behaviorとして扱い、単純に「高いVelocity」として表現しません。

将来のAccent Amountは以下へ影響可能にします。

- Amplitude
- Filter
- Attack
- Decay
- Drive / Saturation

## Note Length

Note Lengthは、Note / Gateがどれだけ継続するかを表します。

内部では`lengthTicks`としてMusical Tickで保持します。v0.1の16分音符Stepは240 ticksなので、例として:

```text
25% Step  = 60 ticks
50% Step  = 120 ticks
75% Step  = 180 ticks
100% Step = 240 ticks
2 Steps   = 480 ticks
4 Steps   = 960 ticks
```

UIではTick値を直接見せず、音楽的に分かりやすい単位で操作します。

初期候補:

- 25%
- 50%
- 75%
- 100%
- 2 Steps
- 3 Steps
- 4 Steps
- Custom

例えば4/4・16分Gridでは:

```text
Step 1: Cmaj7
Length: 4 Steps
```

とすると、Cmaj7は4分音符相当の長さで鳴ります。

Synth、Sampler Gate、Sampler LoopはLengthを解釈します。

One Shot Samplerでは、Note Offによる停止にLengthを使用しなくても構いません。

### ChordのLength

v0.1では1 Step内の`notes[]`は同じLengthを共有します。

つまりChord内の各Noteへ別々のLengthを設定する機能は初期版では持ちません。

将来、必要性が高ければNote単位Eventへ拡張します。

## Tie

Tieは内部で独立した音楽Eventとして保存するのではなく、**前のNote / ChordのLengthを延長する編集操作**として扱う方針です。

例:

```text
Step 1: C3, Length = 1 Step
Step 2: TIE
```

UI上でTIEを指定した結果、内部的には概念上:

```text
Step 1: C3, Length = 2 Steps
Step 2: 新規Triggerなし
```

となります。

Chordでも同様です。

```text
Step 1: Cmaj7
Step 2: TIE
Step 3: TIE
Step 4: TIE
```

は、Step 1のCmaj7を4 Steps分保持する操作として扱います。

この方式により、Tie専用EventをAudio Engineへ渡す必要がなく、Synth、Sampler Gate、将来のMIDIで共通化しやすくなります。

Pattern境界をまたぐTie / Lengthの扱いは、Pattern LoopとPattern切替仕様を決める際に詳細化します。

## Legato

LegatoはTieとは別概念です。

- Tie: 同じEventを長く保持する
- Legato: 次のNoteへ切り替わる際にEnvelope等を再Triggerしない、または滑らかにつなぐ演奏方法

主にMono Synthで重要です。

例:

```text
Step 1: C3
Step 2: D3 + Legato
```

Mono Synthでは、Step 2で新しいPitchへ移行してもAmp Envelopeを完全に再Triggerせず、Glideと組み合わせて滑らかにつなげられます。

v0.1ではLegatoを必須にせず、Mono Synth / Glide実装時に追加できるよう`legato` Fieldを予約します。

Poly SynthやChordに対するLegato Semanticsは複雑になるため、初期版では主にMono Voice向け機能として定義します。

## SwingとMicro Timing

### Swing

初期版:

- Project-level Master Swing

将来:

- Track-level Override / Inherit

### Micro Timing

Micro TimingはMillisecondsではなくMusical Ticksで保存します。

```text
microTimingTicks = -24
```

Scheduling:

```text
scheduledTime = gridTime + swingOffset + microTimingOffset
```

UIでは EARLY ↔ LATE のように簡潔に表示できます。

## Probability

将来のStepごとのProbability:

```text
0–100%
```

再生Passごとに、条件を満たしたTriggerを実際に鳴らすか決定します。

Conditional Triggerとの評価順は今後決定します。

## Conditional Trigger

将来機能。候補:

- First Pass Only
- Every Nth Pass
- Fill Mode
- Previous Step / Result Dependency

Touch UIで扱える程度にCondition Languageを簡潔に保ちます。

## Repeat / Ratchet

1 Step内で複数Triggerを発生させる将来機能です。

検討事項:

- Repeat Count
- Repeat Spacing
- Velocity Shaping
- Probabilityとの関係
- 長いNote Lengthとの関係

## Parameter Lock

Stepは、そのEventについてEngine Parameterの一部をOverrideできます。

```text
parameterLocks = {
  "sampler.start": 0.42,
  "sampler.pitch.coarse": 7,
  "synth.filter.cutoff": 0.38
}
```

Overrideだけを保存し、通常値はEngine / Track側に保持します。

Parameter IDはProject SaveやSoftware Updateをまたいでも安定させます。

## Sample Lock / Sample Override

将来のSampler動作:

```text
Track default sample = snare_01.wav
Step 9 sampleOverride = clap_01.wav
```

1 Track内で複数Sampleを使えるようにします。

## 基本操作

Main Grid:

- StepをTap: Active / Inactive切替
- StepをLong Press: 詳細Step Editor
- Track HeaderをTap: Track / Engine Editor

Synth / Pitched SamplerのStep Editor:

- NOTE: 単音入力
- CHORD: Chord Nameから自動生成
- KEYBOARD: 任意Noteを鍵盤から直接入力
- Length: Gate / Note Length設定
- TIE: 直前EventのLengthを延長
- 将来Legato: Mono Synthの滑らかなNote接続

将来Gesture候補:

- Step上でVertical SwipeしてVelocity調整
- Active Stepを横方向へDragしてLengthを延長

Step Dataが高度になってもGrid自体は視覚的に簡潔に保ちます。

## v0.1で表に出すSequencing Control

推奨:

- Step ON/OFF
- Notes
- Velocity
- Length
- Accent
- Master Swing
- Independent Track Length

Step Resolutionは1/16固定から開始します。

Tieは専用EventではなくLength編集のShortcutとして実装できます。Legatoは内部拡張点を残し、Mono Synth実装時に追加します。

Note入力UIはまずNOTE Modeを実装し、その後CHORD / KEYBOARDを段階的に追加しても構いません。内部の`notes[]` Modelは最初からPolyphonic対応にします。

内部Modelには将来Fieldを保持できる余地を残しますが、未実装のBehaviorを無理に実装しません。
