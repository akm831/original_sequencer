# Architecture

## Purpose

This document defines conceptual boundaries before a concrete framework or programming language is selected.

The central rule is: **musical behavior must not be tightly coupled to a specific UI toolkit or audio backend.**

## Conceptual layers

```text
UI / Interaction
      ↓
Application / Commands
      ↓
Project + Sequencer Model
      ↓
Transport / Musical Clock
      ↓
Track Engine Interface
   ┌──┴──────────┐
Sampler        Synth
   └──┬──────────┘
      ↓
Mixer / FX
      ↓
Audio Output
```

Persistence sits beside the project model and serializes project state without defining musical behavior.

## Major modules

### UI / Interaction

Responsibilities:

- step-grid interaction
- track selection
- sampler waveform editor
- synth controls
- pattern management
- transport controls
- touch gestures

UI code should issue intent/commands to the model and should not contain timing-critical audio scheduling logic.

### Project model

Holds persistent musical state:

```text
Project
├─ bpm
├─ ppqn
├─ masterSwing
├─ sampleLibrary
├─ patterns[]
└─ future: song/arrangement
```

### Pattern

Contains a group of tracks that play together.

```text
Pattern
├─ id
├─ name
└─ tracks[]
```

### Track

A Track is the main musical lane and selects an engine.

```text
Track
├─ id
├─ name
├─ engine
├─ length
├─ rate
├─ swing override (future)
├─ mixer
└─ steps[]
```

Do not create unrelated "drum track" and "melody track" sequencing systems. Sampler and Synth tracks should use the same sequencing core where possible.

### Transport / Clock

Responsibilities:

- play / stop
- BPM
- musical position
- step/bar boundaries
- conversion from musical time to scheduled audio time
- lookahead scheduling strategy

Current timing design target:

- 960 PPQN
- timing stored in musical units/ticks rather than only milliseconds

Scheduling concept:

```text
scheduled musical time
= grid position
+ swing offset
+ micro timing offset
```

The final conversion to seconds/audio frames belongs close to the audio backend.

### Engine abstraction

A Track chooses a sound engine. Initial engine types:

- Sampler
- Synth

Future engine types may include:

- FM synth
- Wavetable synth
- External MIDI

The sequencer should describe musical events and parameter overrides without needing to know how an engine internally produces sound.

### Mixer / FX

Per-track concepts:

- level
- pan
- mute
- solo

Future:

- sends
- insert effects
- master effects

## Event flow

Example: synth note

```text
Step becomes due
→ Sequencer evaluates probability/condition (when implemented)
→ Step note event is generated
→ Parameter locks are applied for this event
→ Synth engine receives note + parameters
→ Mixer
→ Output
```

Example: sampler trigger

```text
Step becomes due
→ Sequencer evaluates step
→ Sampler receives note/trigger
→ Sample selection/start/end/pitch parameters are resolved
→ Voice is played
→ Mixer
→ Output
```

## Parameter model

Future Parameter Lock support requires stable parameter identifiers. Conceptually:

```text
sampler.start
sampler.pitch.coarse
sampler.filter.cutoff
synth.osc1.wave
synth.filter.cutoff
synth.lfo.amount
```

A Step should store only overridden parameters. Track/engine state provides defaults.

## Non-destructive audio-file policy

Sample editing metadata (start/end, loop markers, reverse mode, root note, etc.) should normally be stored in the project without modifying the source audio file.

Destructive/export operations such as Crop or Normalize should be explicit separate actions if introduced later.

## Platform strategy

No platform is fixed yet. The architecture should permit a shared application/sequencer layer with an audio backend that can become native if latency or DSP requirements demand it.

Potential technology choices should be evaluated only after the core musical model is sufficiently specified.
