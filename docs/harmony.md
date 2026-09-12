# Harmony Engine — 将来仕様

## 目的

高度な音楽理論を知らなくても、音楽的に使いやすいHarmony素材を作れるようにします。一方で、最終的な内部表現は通常のNote Eventとして扱います。

Harmony Engineは個別のSound Engineより上位に置き、生成したNoteをSynth、Pitched Sampler、将来のMIDI Outputへ送れるようにします。

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

## 将来のChord Vocabulary

候補:

- major
- minor
- 7
- m7
- maj7
- dim
- aug
- sus2
- sus4
- m7b5
- 9
- m9
- maj9

最初の実装では種類を絞って構いません。

## Chord Event Metadata

概念表現:

```text
ChordEvent
├─ root
├─ chordType
├─ inversion
├─ octave
├─ voicing
└─ generatedNotes[]
```

Chord MetadataとGenerated Notesを両方永続化するか、Metadataから毎回再生成するかは未決定です。

## Voicing

将来候補:

- Root Position
- Inversions
- Open Voicing
- Drop Voicings

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

## Arpeggiator

Chord Notesを時間方向のNote Patternへ変換する将来Moduleです。

可能な限り通常のSequencer Note Eventを出力する構造にします。

## UI原則

Harmony UIは音楽理論の理解を要求するのではなく、その負担を減らす方向で設計します。高度なVoicing ControlはDetail Viewへ分離できます。
