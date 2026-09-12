# Sampler Specification

## Goals

The Sampler must work for short drum hits and longer musical/field-recording samples. Editing is non-destructive by default.

## Initial parameter set

```text
Sample
├─ file reference
├─ start
├─ end
├─ reverse
└─ root note

Playback
├─ One Shot
├─ Gate
└─ Loop

Pitch
├─ coarse tune
└─ fine tune

Amp
├─ attack
├─ decay
├─ sustain
└─ release

Filter
├─ type
├─ cutoff
└─ resonance
```

## Waveform editor

The sample editor should prominently display the waveform with draggable Start/End markers.

Touch interaction should support:

- moving Start/End markers
- zooming into the waveform later
- audition/playback

For waveform rendering, use a reduced peak representation suitable for screen width rather than drawing every PCM sample directly.

## Playback modes

### One Shot

A trigger plays the selected sample range through its end. Step note length does not stop playback.

Suitable for drums and percussion.

### Gate

Playback lasts while the note/gate is active, subject to the amp envelope.

Suitable for melodic samples, vocal fragments, etc.

### Loop

A selected region repeats while the voice remains active.

Initial implementation may reuse Start/End as the loop region; dedicated Loop Start/End markers can be added later.

## Pitch

Initial target:

- Coarse: at least ±24 semitones
- Fine: ±100 cents

The sampler should support key tracking when a root note is defined.

Example:

```text
root note = C3
C3 -> original pitch
E3 -> +4 semitones
G3 -> +7 semitones
```

This permits simple melodic sampler use and future chord playback.

## Reverse

Reverse should change playback direction non-destructively. The source sample file remains unchanged.

## Filter

Initial types:

- Low Pass
- High Pass

Band Pass may be added if implementation cost is low.

The filter subsystem should be conceptually reusable by Synth where practical.

## Parameter Lock integration

Future P-Lock targets include at minimum:

- sample start
- sample end (if musically useful)
- pitch coarse/fine
- reverse
- amp envelope parameters
- filter cutoff
- filter resonance

## Sample override / Sample Lock

A Track has a default sample, but future Steps may reference a different sample.

```text
Track default: snare_01.wav
Step 9 override: clap_01.wav
```

## Slice — future

A sample can later contain named/numbered slices:

```text
Slice 1: start/end
Slice 2: start/end
...
```

Future slice creation modes:

- equal divisions
- transient detection
- beat grid

A Step may select a slice directly.

## Multisample — future

The data model should not permanently assume one sample per pitched instrument. A future multisample map may choose different source samples across key/velocity ranges.

## Recording — future

Desired workflow:

```text
Microphone/Input
→ Record
→ Waveform
→ Trim
→ Assign to Sampler
→ Sequence
```

Not required for the first implementation milestone.

## Time Stretch — future

Time stretching is distinct from simple pitch/rate change and is explicitly deferred because of its higher DSP complexity.

## File safety and licensing

Imported user samples remain user assets. Bundled factory samples must have redistribution-safe licensing documented by the project.
