# Harmony Engine — Future Specification

## Goal

Allow users to create harmonically useful material without requiring advanced theory knowledge, while keeping the resulting music represented as normal note events.

The Harmony Engine belongs above individual sound engines. It should generate notes that can be sent to Synth, pitched Sampler, or future MIDI output.

## Chord generation

User selects a root and chord type:

```text
Root: C
Chord Type: m7
```

The engine expands this to notes:

```text
C, Eb, G, Bb
```

The Step ultimately stores/plays note events rather than a synth-specific chord preset.

## Future chord vocabulary

Examples:

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

The first implementation can expose a smaller set.

## Chord event metadata

Conceptual representation:

```text
ChordEvent
├─ root
├─ chordType
├─ inversion
├─ octave
├─ voicing
└─ generatedNotes[]
```

Whether chord metadata is persisted alongside generated notes or regenerated from metadata remains an open implementation decision.

## Voicing

Future options include:

- root position
- inversions
- open voicing
- drop voicings

The purpose is musical control, not exhaustive academic coverage.

## Project key / scale

Future Project state:

```text
key = C
scale = Minor
```

This enables scale-aware suggestions and input constraints.

## Diatonic assistance

For a selected key/scale, the UI may show valid diatonic chords for rapid entry.

Example for C minor:

```text
Cm | Ddim | Eb | Fm | Gm | Ab | Bb
```

## Chord progression assistance

Future feature:

- suggest likely next chords based on current key and previous chord
- suggestions must remain optional; the user is always free to choose any chord

## Voice leading

Future automatic voicing can minimize note movement between adjacent chords.

Example progression:

```text
Cmaj7 → Am7 → Fmaj7 → G7
```

Instead of always using root position, the engine can choose inversions that produce smoother movement.

## Arpeggiator

Potential future module that transforms chord notes into timed note patterns.

It should consume note/chord information and emit ordinary sequencer note events where practical.

## UI principle

The harmony UI should reduce theory friction rather than require users to understand chord formulas. Advanced voicing controls can remain behind a detail view.
