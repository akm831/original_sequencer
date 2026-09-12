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

すべてを最初からUIへ並べる必要はなく、主要ChordをPrimary View、その他をDetail Viewへ分けても構いません。

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

Chord MetadataとGenerated Notesを両方永続化するか、Metadataから毎回再生成するかは未決定です。

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
