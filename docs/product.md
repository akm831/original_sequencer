# Product Specification

## Working definition

The product is an original **groovebox-style music production application** built around a fast step sequencer, sample playback/editing, and synthesis.

It should be capable of making a useful musical sketch without requiring external instruments.

## Design goals

1. **Immediate** — a user should be able to create a beat quickly with a 16-step workflow.
2. **Deep when needed** — advanced per-step controls should exist behind a simple surface.
3. **Touch-first** — the main workflow should remain usable on phones/tablets while also working with mouse/desktop input.
4. **Engine-flexible** — tracks choose a sound engine rather than being permanently designated as drums or melody.
5. **Extensible** — the core data model should support future probability, parameter locks, conditional triggers, slicing, harmony tools, MIDI, and song mode.
6. **Non-destructive** — sample start/end and similar edits should normally change playback metadata rather than rewrite the original sample file.

## Inspiration, not cloning

The project draws conceptual inspiration from several sequencer/groovebox traditions:

- TR-style fast step entry and visual clarity
- Elektron-style per-step variation and parameter locking
- modern groovebox independent track lengths/rates
- MPC/Digitakt-style waveform-oriented sampling
- DAW-style internal event flexibility where useful

The goal is not to recreate any specific commercial device.

## Initial useful version

### Tracks

- Target: approximately 8 tracks
- Each track selects an engine:
  - Sampler
  - Synth
- Track length: 1–16 steps initially
- Future: independent track playback rate

### Sequencing

- 16-step primary grid
- Note / notes
- Velocity
- Note length
- Accent
- Master swing
- Multiple patterns (initial target: 8)

### Sampler

- Import common PCM audio files such as WAV/AIFF where supported
- Waveform display
- Non-destructive Start / End
- One Shot / Gate / Loop playback concepts
- Coarse/fine pitch
- Reverse
- Root note metadata
- Amp envelope
- Basic filter

### Synth

Initial synthesis model: subtractive synthesizer.

- 2 oscillators
- Sine / triangle / saw / square
- Noise
- Filter
- Amp ADSR
- Filter ADSR
- LFO
- Mono / poly mode
- Glide
- Presets

### Factory content

Initial target:

- roughly 70–100 drum / percussion / FX samples
- roughly 30 synth presets

All bundled content must have licensing that explicitly permits redistribution inside the application.

## Future capabilities

These are planned extension points, not necessarily v0.1 requirements:

- per-step probability
- micro timing
- parameter locks
- conditional triggers
- ratchet / repeat
- per-track swing
- independent track rate
- sample lock / per-step sample override
- slicing and transient detection
- time stretching
- multisampling
- recording
- external MIDI
- additional synth engines (FM, wavetable, etc.)
- project key / scale
- chord generator
- inversions and voicings
- voice-leading assistance
- arpeggiator
- pattern chaining / song mode
- mixer effects and master effects

## Product hierarchy

```text
Project
├─ Global musical settings
├─ Sample library
├─ Patterns
│  └─ Tracks
│     ├─ Engine
│     ├─ Mixer settings
│     └─ Steps / events
└─ Future song / arrangement data
```

## Main-screen principle

The primary screen should remain focused on playing and programming steps. Deeper sound design and event editing should open in dedicated views rather than overcrowding the grid.

## Open product questions

- Final target platforms
- Final application framework
- Native vs shared audio engine strategy
- Exact maximum track and polyphony limits
- Pattern length beyond 16 steps
- Scope of effects in the first release
- Whether recording is required before the first public release
