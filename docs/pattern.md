# Pattern仕様

## 目的

Patternは、複数TrackのStep DataとEngine Stateをまとめて保持し、Groovebox上で素早く切り替えられる演奏単位とします。

v0.1では16-Step Grooveboxとしての分かりやすさを優先しつつ、将来32 / 64 Steps、Pattern Chain、Song Modeへ拡張できる構造を維持します。

## 基本構造

概念上のPatternは以下を保持します。

```text
Pattern
├─ id
├─ name
├─ tracks[]
└─ future metadata
```

各TrackはPattern内に存在し、Engine Type / Engine State / Mixer State / Stepsを保持します。

```text
Pattern
└─ Tracks[]
   └─ Track
      ├─ engineType
      ├─ engineState
      ├─ mixerState
      ├─ length
      └─ steps[]
```

このためPatternを複製すると、StepだけでなくそのPatternで使用しているSound Stateも複製される構造を基本とします。

## v0.1のPattern長

v0.1では、Main Gridの基準を以下に固定します。

- 1 Step = 16分音符
- 16 Global Steps = 1小節（4/4）
- Pattern Launch Quantization = 1小節

ただしTrack LengthはPatternのGlobal 16 Stepsとは独立しています。

例:

```text
Kick  = 16 Steps
Snare = 16 Steps
Hat   = 15 Steps
Perc  = 7 Steps
```

15 Stepや7 StepのTrackは独自周期でループしますが、Pattern切替予約の基準は各Trackの終端ではなくProject Transportの小節境界とします。

## Pattern切替

演奏中に別Patternを選択した場合、標準動作は即時切替ではなく**次の小節境界で切替予約**とします。

例:

```text
現在: Pattern A
User: Pattern BをTap
状態: Pattern B = QUEUED
次の小節先頭:
Pattern A → Pattern B
```

これによりLive Performance中でもBeat途中で突然Patternが切れにくくなります。

### UI State

Pattern Buttonは少なくとも以下の3状態を区別できるようにします。

- Current: 現在再生中
- Queued: 次に切り替わる予約済みPattern
- Idle: その他

概念UI:

```text
[A] CURRENT   [B] QUEUED   [C]   [D]
```

停止中はPatternを即座に選択して構いません。

## Pattern切替時のTrack Phase

新しいPatternへ切り替えるときは、全Trackを新PatternのStep 1から開始する方針とします。

つまり旧PatternでHatが15-Step Loopの途中にいても、そのPhaseを新Patternへ引き継ぎません。

```text
Pattern A Hat phase = Step 11 / 15
↓ Pattern switch
Pattern B Hat phase = Step 1
```

Patternは独立した音楽状態として扱い、切替結果を予測しやすくします。

将来、Continuous Phaseを必要とするUse Caseが明確になれば別Optionとして検討します。

## BPM

BPMはPatternごとではなくProject Globalとします。

```text
Project
└─ bpm
```

Patternを切り替えてもTempoは変化しません。

理由:

- Live Performanceで予期しないTempo Jumpを防ぐ
- Pattern Chain / Song Modeを単純化する
- SwingやTransport SchedulingをProject Clockへ集約できる

将来Tempo AutomationやSong SectionごとのTempo Changeが必要になった場合は、Arrangement Layerで扱う方針を優先します。

## Pattern Copy / Duplicate

Pattern作成ではBlankから毎回作るより、既存Patternを複製してVariationを作れることが重要です。

v0.1では最低限以下を用意します。

- New / Empty Pattern
- Duplicate Pattern
- Clear Pattern

DuplicateはPattern全体を複製します。

対象:

- 全TrackのSteps
- Track Length
- Engine Type
- Engine State
- Mixer State

例:

```text
Pattern A
↓ Duplicate
Pattern B
↓ Snare / Chordだけ変更
Variation完成
```

Track単位、Step Range単位のCopy / Pasteは将来追加可能とします。

## Pattern Loopと長いNote

Note LengthはPattern境界をまたげる内部表現を維持します。

同じPatternをLoopしている場合、境界をまたぐNoteは指定Lengthまで保持可能とします。

ただしLoop先の新しいTriggerは通常どおり発生します。その結果、Poly Synthでは一時的にVoiceが重なる可能性があります。

具体的なVoice Stealing RuleはAudio Engine仕様で決定します。

## Pattern切替と鳴り残り

別Patternへ切り替える境界では、旧Pattern由来のGate保持中NoteにはNote Offを送る方針とします。

ただしAudio Tailまで強制的に瞬断しません。

例:

- Synth: Note Off後のRelease Tailは残る
- Sampler Gate / Loop: Gateを終了
- Sampler One Shot: 原則として再生中のSample Tailを許容
- Delay / Reverb: 将来的にはTailを自然に残せる設計を優先

これによりPattern切替時に不自然なHard Cutを減らしつつ、旧Patternの長音が無制限に新Patternへ侵入することを防ぎます。

## Pattern数

v0.1の初期目標は約8 Patternsです。

UI例:

```text
[A] [B] [C] [D]
[E] [F] [G] [H]
```

内部データ構造は8固定にせず、将来Pattern Bankやより多くのPatternへ拡張できるようにします。

## 将来の32 / 64 Step対応

v0.1のMain Surfaceは16 Stepsを維持します。

長いPatternが必要になった場合、Main Gridそのものを巨大化するのではなく**Page方式**を基本候補とします。

```text
Page 1 = Steps 1–16
Page 2 = Steps 17–32
Page 3 = Steps 33–48
Page 4 = Steps 49–64
```

UI例:

```text
PAGE [1] [2] [3] [4]
```

これによりスマートフォンでも各Step ButtonのTouch Targetを小さくしすぎずに済みます。

ただしIndependent Track Lengthとの整合が必要なため、32 / 64 Step対応時には以下を再検証します。

- Trackごとの最大Length
- Track PageとPattern Pageの関係
- Pattern Launch Quantization
- Polymetric Trackの表示

## Pattern Chain

将来、複数Patternを一時的に連続再生できるPattern Chainを追加します。

例:

```text
A → A → B → C
```

Pattern ChainはPattern自体を書き換えず、再生順だけを保持します。

Live Jamや簡単な曲展開を作る用途を想定します。

## Song / Arrangement Mode

さらに将来はPattern Chainを発展させ、SectionやRepeat Countを持つArrangementを追加可能にします。

概念例:

```text
Intro   A x4
Verse   B x8
Chorus  C x8
Break   D x4
```

Song ModeはPattern Sequencerより上位Layerに置き、Pattern内部のStep Modelを変更しない構造とします。

## v0.1で表に出すPattern Control

推奨:

- Pattern Select
- Current / Queued表示
- Duplicate
- Clear / New
- 約8 Pattern Slots
- 再生中は次の小節でQuantized Switch

v0.1では不要:

- 32 / 64 Step Page
- Pattern Chain
- Song Mode
- PatternごとのBPM
- Arbitrary Launch Quantization

内部Architectureはこれらを後から追加できるようにします。
