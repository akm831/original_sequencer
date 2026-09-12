# AGENTS.md

This repository is a specification-first music software project.

## Current phase

The project is still defining product behavior and architecture. Do not assume that a framework, language, UI toolkit, or audio backend has been finalized unless a later decision document explicitly says so.

## Source of truth

Before implementing anything, read:

1. `docs/product.md`
2. `docs/architecture.md`
3. `docs/sequencer.md`
4. The relevant engine specification (`docs/sampler.md`, `docs/synth.md`, `docs/harmony.md`)
5. `docs/decisions.md`
6. `docs/roadmap.md`

If code and documentation disagree, do not silently choose one. Prefer the documented behavior, identify the mismatch, and update the specification when the design intentionally changes.

## Architectural constraints

Keep these concerns separated:

- UI / interaction
- transport and musical clock
- sequencer model
- track / pattern / project model
- sound engine abstraction
- sampler engine
- synth engine
- mixer / effects
- persistence

The sequencer must not depend directly on a specific UI framework or audio backend.

A Track should select an audio engine rather than being permanently defined as either a drum track or melodic track.

Step data must be extensible enough to support future probability, micro timing, conditional triggering, repeat/ratchet, sample locks, and parameter locks.

Musical timing should use a high-resolution musical time representation rather than storing scheduling offsets only in milliseconds. The current design target is 960 PPQN unless deliberately changed.

## Implementation style

Work in small, testable increments. Prefer a playable vertical slice over broad scaffolding.

Suggested implementation order:

1. project skeleton
2. transport / musical clock
3. one 16-step track
4. one sound source
5. multiple tracks
6. track length
7. persistence
8. sampler waveform/editor
9. synth engine
10. advanced sequencing

Do not add advanced abstractions merely because they might be useful later. Preserve extension points in the data model, but keep the first implementation understandable.

## Product principles

- Touch-first UI
- Immediate music making
- Simple main screen, deeper edit screens
- Non-destructive sample editing by default
- Sampler and synth should share sequencing semantics where possible
- Chords are represented as musical note events, not baked into synth presets
- Factory content must have redistribution-safe licensing

## When making design choices

Record meaningful decisions in `docs/decisions.md`, especially when choosing:

- framework / language
- audio backend
- supported platforms
- persistence format
- timing model
- polyphony limits
- parameter automation / parameter lock representation
- sample file handling

## Coding-agent behavior

When asked to implement a feature:

- inspect the relevant specifications first;
- state any material assumptions;
- avoid unrelated refactors;
- add or update tests where practical;
- keep commits focused;
- update documentation when user-visible behavior changes.
