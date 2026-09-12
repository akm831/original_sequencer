# Sequencer Specification

## Core hierarchy

```text
Project
└─ Patterns[]
   └─ Pattern
      └─ Tracks[]
         └─ Track
            └─ Steps[]
               └─ Step
```

The sequencer is shared by sampler and synth tracks.

## Project timing

Initial target:

```text
bpm
ppqn = 960
masterSwing
```

Swing should be represented musically rather than as a fixed millisecond offset.

Suggested UI convention:

- 50% = straight
- higher values delay the appropriate subdivisions

## Track

Conceptual Track data:

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

### Track length

Initial implementation target:

- 1–16 steps independently per track

Example:

```text
Kick  16
Snare 16
Hat   15
Perc   7
```

This intentionally enables evolving polymetric patterns.

### Track rate

Future target options may include:

- 1/4x
- 1/2x
- 1x
- 2x
- 4x

Do not require this in the first playable milestone.

## Step

A Step is not just an ON/OFF boolean. It is an extensible musical event container.

Conceptual data:

```text
Step
├─ enabled
├─ notes[]
├─ velocity
├─ length
├─ accent
├─ microTimingTicks
├─ probability
├─ condition
├─ repeat
├─ sampleOverride
└─ parameterLocks
```

Not every field must be exposed in v0.1.

## Notes

Use a collection rather than only a single note so chords are representable.

Examples:

```text
Bass note:
notes = [C2]

Chord:
notes = [C3, E3, G3]
```

This representation should work regardless of whether notes drive the built-in Synth, a pitched Sampler, or a future MIDI engine.

## Velocity and Accent

Velocity and Accent are separate concepts.

### Velocity

Internal target range:

- 1–127

This is convenient for future MIDI interoperability.

Velocity represents event strength.

### Accent

Accent is a separate emphasis flag / behavior. It must not simply be encoded as "high velocity".

Future Accent Amount may influence several parameters, e.g.:

- amplitude
- filter
- attack
- decay
- drive / saturation

## Note length

Length determines how long a note/gate remains active where the engine respects gates.

Examples:

- Synth notes
- Sampler Gate playback
- Sampler Loop playback

One Shot sampler playback may intentionally ignore note length for stopping behavior.

## Swing and Micro Timing

### Swing

Initial version:

- Project-level master swing

Future:

- Track-level override/inherit

### Micro timing

Store micro timing in musical ticks, not milliseconds.

Example:

```text
microTimingTicks = -24
```

Scheduling concept:

```text
scheduledTime = gridTime + swingOffset + microTimingOffset
```

UI can present this simply as EARLY ↔ LATE.

## Probability

Future per-step probability:

```text
0–100%
```

Probability determines whether an otherwise eligible trigger occurs on a playback pass.

Exact interaction with conditional triggering is still to be specified.

## Conditional Trigger

Future functionality. Examples may include:

- first pass only
- every Nth pass
- fill mode
- previous-step/result dependency

The exact condition language should remain simple enough for touch UI.

## Repeat / Ratchet

Future per-step repeated triggers inside a step interval.

Important future considerations:

- repeat count
- repeat spacing
- velocity shaping
- probability interaction

## Parameter Locks

A Step may override a subset of current engine parameters for that event.

Conceptually:

```text
parameterLocks = {
  "sampler.start": 0.42,
  "sampler.pitch.coarse": 7,
  "synth.filter.cutoff": 0.38
}
```

Only overrides should be stored. The engine/track retains the normal parameter value.

Parameter IDs need to remain stable across project saves and software updates.

## Sample Lock / Sample Override

Future sampler behavior:

```text
Track default sample = snare_01.wav
Step 9 sampleOverride = clap_01.wav
```

This allows multiple samples on one sequencer track without duplicating the track.

## Main interaction model

Primary grid:

- tap step: toggle active/inactive
- long press step: open detailed step editor
- tap track header: open track/engine editor

Potential future gesture:

- vertical swipe on a step to adjust velocity

The grid should stay visually simple even as Step data becomes richer.

## v0.1 exposed sequencing controls

Recommended first exposure:

- Step ON/OFF
- notes
- velocity
- length
- accent
- master swing
- independent track length

The internal model may reserve fields for later features without implementing their behavior yet.
