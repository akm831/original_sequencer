# Harmony Engine — 将来仕様

## 目的

高度な音楽理論を知らなくても、音楽的に使いやすいHarmony素材を作れるようにします。一方で、最終的な内部表現は通常のNote Eventとして扱います。

Harmony Engineは個別のSound Engineより上位に置き、生成したNoteをSynth、Pitched Sampler、将来のMIDI Outputへ送れるようにします。

## 初期方針

最初のHarmony機能では、**最大4音程度のChordを簡単に入力・再生できること**を優先します。

初期UIではTriadと7th系Chordを中心に扱い、複雑なTension指定や高度なVoicingはDetail Viewまたは将来機能へ分離します。

一方、内部データモデルは4音固定にせず、将来以下のようなExtended Chordへ拡張できる構造にします。

- 9th
- 11th
- 13th
- add9 / add11
- altered tension（b9、#9、#11、b13等）

これにより、初期UIを単純に保ちながら後からJazz、Neo Soul、House、R&B等で使われるTension Chordへ拡張できます。

## Sequencer上の入力Mode

Harmony EngineはStep Editorの`CHORD` Modeから利用します。

```text
[ NOTE ] [ CHORD ] [ KEYBOARD ]
```

- NOTE: 単音を直接指定
- CHORD: Root / Chord Type等からHarmony EngineがNoteを生成
- KEYBOARD: 任意のNote Combinationを手動入力

3つは別々のEvent Typeではなく、同じ`Step.notes[]`を作るための入力方法です。

これにより、Harmony機能を使わないUserやChordを手動で調整したいUserも同じSequencer Modelを利用できます。

## Chord生成

UserがRootとChord Typeを選択します。

```text
Root: C
Chord Type: m7
```

Harmony Engineが以下のNoteへ展開します。

```text
C, Eb, G, Bb
```

StepはSynth固有のChord Presetではなく、通常のNote Eventとして保存・再生します。

## 初期CHORD Mode UI

スマートフォンでは、最初の画面に必要なControlを絞ります。

```text
CHORD

Root      C
Type      m7
Inversion Root
Octave    3

Notes     C3 Eb3 G3 Bb3

[AUDITION]
```

基本項目:

- Root
- Chord Type
- Octave
- Inversion
- Generated Notes Preview
- Audition

Step共通項目としてVelocity / Length / Accentも編集できます。

Chordを選んだ瞬間にPreview再生できると、音楽理論の知識が少なくても耳で選択できます。

## 初期Chord Vocabulary

初期版では、4音以内で実用性の高いChordを優先します。

候補:

- major
- minor
- dim
- aug
- sus2
- sus4
- 7
- m7
- maj7
- m7b5

すべてを最初からUIへ並べる必要はありません。

Primary ViewにはMajor / Minor / 7 / m7 / maj7等の頻用Chordを置き、その他は追加Menuへ分離する設計が適しています。

## Inversion

初期段階からInversionを扱える設計とします。

4音Chordの場合の例:

```text
Root Position
1st Inversion
2nd Inversion
3rd Inversion
```

ただしTriadでは存在しないInversionを表示しない等、Chordの構成音数に合わせてUIを変えます。

初期実装は単純なOctave Rotationで構いません。高度なVoice LeadingやOpen Voicingは将来機能とします。

## KEYBOARD Modeとの関係

CHORD Modeで生成したNotesは、将来的にKEYBOARD Modeで個別に調整できるようにします。

例:

```text
CHORD: Cmaj7
Generated: C3 E3 G3 B3

KEYBOARDで調整:
C3 G3 B3 E4
```

これにより、Chord Nameから始めてから好みのVoicingへ編集できます。

その際、元のChord Metadataを維持するか、手動編集として切り離すかは実装時に明確なRuleを定めます。

## 将来のExtended Chord / Tension

将来的にはChord Typeを単なる固定名称だけでなく、**基本Chord + Extension / Tension**として表現できる設計を検討します。

例:

```text
Root: C
Base: m7
Extensions: 9, 11
```

生成例:

```text
C, Eb, G, Bb, D, F
```

別例:

```text
Root: G
Base: 7
Tensions: b9, 13
```

この構造により、`Cm9`、`Cmaj9`、`C11`、`C13`、`G7(b9,13)`等を将来扱えるようにします。

UIでは理論用語を過度に要求せず、Preset的なChord Name選択とAdvanced Tension編集の両方を可能にする方向を検討します。

## Chord Event Metadata

概念表現:

```text
ChordEvent
├─ root
├─ chordType
├─ extensions[]
├─ alterations[]
├─ inversion
├─ octave
├─ voicing
└─ generatedNotes[]
```

`extensions[]`と`alterations[]`は将来機能ですが、データモデル上は追加可能な構造を保ちます。

再生側のAudio EngineはChordEventを解釈せず、最終的な`generatedNotes[]` / `Step.notes[]`を受け取ります。

Chord Metadataは、Chord Nameで後から再編集したりHarmony支援を行うための補助情報として扱います。

Chord MetadataとGenerated Notesを両方永続化するか、Metadataから毎回再生成するかは、Project Format設計時に最終決定します。

## 発音数との関係

初期Harmony UIは最大4音程度を基本とします。

Synth v0.1では1 Trackあたり最大8 Voiceを暫定目標としているため、将来的には5〜7音程度のExtended Chordも理論上扱えます。ただしRelease中のVoiceやVoice Stealingとの競合があるため、実際の最大Chord構成音数はAudio Engine検証後に決定します。

Harmony Engine自体は4音固定にせず、任意数のGenerated Notesを扱える設計にします。

## Voicing

将来候補:

- Root Position
- Inversions
- Open Voicing
- Drop Voicings
- Tensionを含むSpread Voicing
- 一部Noteの省略（Omit）

Tension Chordでは構成音をすべて鳴らすとは限らないため、将来的には3rd、5th、Root等を意図的に省略できる仕組みも検討します。

網羅的な音楽理論機能ではなく、実際の作曲に使いやすいControlを優先します。

## Project Key / Scale

将来のProject State:

```text
key = C
scale = Minor
```

これによりScale-awareなSuggestionや入力制限を可能にします。

## Diatonic支援

選択中のKey / Scaleに対し、Diatonic ChordをUI上に候補表示できるようにします。

C minorの例:

```text
Cm | Ddim | Eb | Fm | Gm | Ab | Bb
```

将来的にはCHORD Mode内に、Chord Type一覧とは別に`IN KEY`の候補列を用意することも検討します。

## Chord Progression支援

将来機能:

- 現在のKeyと直前のChordをもとに次のChord候補を提示
- Suggestionは常に任意とし、Userは自由に任意のChordを選択可能

## Voice Leading

将来的には、隣接Chord間のNote移動量が小さくなるようInversion / Voicingを自動選択できるようにします。

```text
Cmaj7 → Am7 → Fmaj7 → G7
```

常にRoot Positionを使うのではなく、より滑らかなVoice Movementを作れるようにします。

Tension Chordでも、共通音を保持したり最小移動となるVoicingを選択できる設計を目指します。

## Arpeggiator

Chord Notesを時間方向のNote Patternへ変換する将来Moduleです。

可能な限り通常のSequencer Note Eventを出力する構造にします。

## UI原則

Harmony UIは音楽理論の理解を要求するのではなく、その負担を減らす方向で設計します。

初期UIでは4音程度までのChordを素早く選べることを優先し、高度なTension、Alteration、Voicing ControlはDetail Viewへ分離します。

Chord Nameだけでなく生成Noteを常に確認でき、可能ならAuditionで即座に試聴できるようにします。
