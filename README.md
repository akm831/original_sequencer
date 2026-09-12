# Original Sequencer

A groovebox-style music production app centered on a fast step sequencer, sampler, and synthesizer.

The project is currently in the **specification and architecture phase**. Implementation technology and target platform are intentionally not locked yet.

## Product direction

The core idea combines:

- TR-style immediacy and a simple 16-step workflow
- Elektron-style per-step depth such as parameter locks and probability
- Flexible track length/rate inspired by modern grooveboxes
- Built-in sampler with waveform editing
- Built-in synthesizer for melodic and harmonic parts
- A future harmony engine for chord generation and scale-aware assistance
- Touch-first interaction so the design can scale to mobile devices

## Initial product target

The first useful version should behave as a compact groovebox rather than only a drum machine.

- Around 8 tracks
- Per-track engine selection: Sampler or Synth
- 16-step sequencing
- Independent track length
- Velocity, accent, note length, swing
- Waveform-based sample editing
- Basic subtractive synthesis
- Multiple patterns
- Factory samples and synth presets

Advanced functions such as probability, micro timing, parameter locks, conditional triggers, slicing, song mode, and MIDI are designed as extensions rather than assumptions of the first implementation milestone.

## Documentation

- [`AGENTS.md`](AGENTS.md) — instructions for Codex and coding agents
- [`docs/product.md`](docs/product.md) — product vision and scope
- [`docs/architecture.md`](docs/architecture.md) — conceptual architecture
- [`docs/sequencer.md`](docs/sequencer.md) — sequencer data model and behavior
- [`docs/sampler.md`](docs/sampler.md) — sampler requirements
- [`docs/synth.md`](docs/synth.md) — synthesizer requirements
- [`docs/harmony.md`](docs/harmony.md) — future chord / harmony engine
- [`docs/roadmap.md`](docs/roadmap.md) — staged implementation plan
- [`docs/decisions.md`](docs/decisions.md) — design decisions and open questions

## Development policy

Do not start by building a large application all at once. Implement vertical slices that can be played and tested early. Product behavior in `docs/` is the source of truth; when implementation decisions conflict with it, update the relevant specification deliberately rather than silently changing behavior.
