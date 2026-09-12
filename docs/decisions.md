# Design Decisions and Open Questions

This file records decisions that should remain stable unless deliberately revisited.

## Confirmed decisions

### Product identity

The project is a groovebox-style music production application, not only a drum-machine clone.

### Primary interaction

Use a simple 16-step surface as the main sequencing interaction. Deeper controls belong in detail views.

### Track model

Tracks select an engine. Initial engine types are Sampler and Synth. Do not build separate unrelated drum and melodic sequencer cores.

### Step model

A Step must be extensible and capable of carrying note events, including multiple notes for chords.

### Velocity vs Accent

Velocity and Accent are distinct musical concepts and should be represented separately.

### Musical timing

Store timing in musical units rather than only milliseconds. Current design target: 960 PPQN.

### Swing vs Micro Timing

Swing is systematic groove timing; Micro Timing is a per-step offset. Keep them separate.

### Track length

Independent per-track length is a core capability. Initial target range: 1–16 steps.

### Sampler editing

Sample Start/End, reverse, root note, and similar operations are non-destructive by default.

### Synth direction

The first built-in synth should be a compact subtractive synthesizer rather than integrating a very large third-party instrument.

### Chord generation

Future chord/harmony features belong above the audio engine and generate ordinary notes. They should not be implemented merely as special synth presets.

### Factory content

Bundled samples/presets must have clear redistribution-safe licensing.

## Tentative decisions

These are current preferences but may be revisited after prototyping.

- Approximately 8 tracks for the initial groovebox target
- Approximately 8 patterns initially
- Around 70–100 factory samples
- Around 30 synth presets
- Master Swing in v0.1; per-track swing later
- Track rate changes after independent track length is proven

## Open questions

### Platform / framework

Not yet chosen.

Candidates previously considered include:

- Flutter plus native audio integration if needed
- Web technologies plus a native/mobile wrapper
- Native/JUCE/C++ audio layers for higher-performance DSP

The decision should be made based on product requirements, not convenience alone.

### Audio engine

Open questions:

- fully shared engine vs platform-native layer
- sample-accurate scheduling strategy
- maximum polyphony
- voice stealing
- resampling/interpolation quality
- filter implementation
- effects architecture

### Project format

Need to decide:

- serialization format
- explicit schema versioning
- asset references and portable project bundles
- missing-sample behavior
- migrations between versions

### Step semantics

Still to define in detail:

- probability interaction with conditional triggers
- repeat/ratchet order of evaluation
- parameter-lock lifetime and interpolation behavior
- parameter locks on polyphonic notes
- sample lock interaction with sampler parameter locks

### Pattern behavior

Still to define:

- pattern switching quantization
- per-pattern vs global BPM
- pattern copy/duplicate workflow
- maximum pattern length beyond 16

### Sampler

Still to define:

- exact loop behavior
- Start/End units and precision
- envelope behavior for One Shot mode
- slice representation
- sample caching strategy
- supported file formats by platform

### Synth

Still to define:

- initial polyphony limit
- oscillator anti-aliasing strategy
- exact filter types
- LFO sync/free modes
- preset format

### Harmony

Still to define:

- persisted chord metadata vs generated notes only
- first chord vocabulary
- scale model
- chord suggestion behavior
- voicing constraints

## Decision log format

When a major technical or product decision is made, add an entry:

```text
YYYY-MM-DD — Decision title
Decision:
Reason:
Alternatives considered:
Consequences:
```

This keeps later Codex sessions from re-deciding settled architecture without context.
