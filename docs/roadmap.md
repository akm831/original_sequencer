# Roadmap

This roadmap is intentionally staged. The objective is to reach playable vertical slices early and avoid building a large untestable architecture.

## Phase 0 — Specification

Status: current phase.

Deliverables:

- product definition
- conceptual architecture
- sequencer model
- sampler requirements
- synth requirements
- harmony future requirements
- technology/platform decision later

Exit criteria:

- enough behavior is specified that an implementation prototype can be judged against clear expectations

## Phase 1 — Minimal playable sequencer

Goal: prove timing and interaction.

Scope:

- application skeleton
- transport: play / stop
- BPM
- 16-step grid
- one track
- one simple sound source
- reliable lookahead scheduling
- visual playhead

No advanced sound design or persistence required yet.

## Phase 2 — Core groovebox structure

Goal: prove the Track/Pattern model.

Scope:

- multiple tracks (target toward 8)
- track mute/solo/level
- independent track length 1–16
- velocity
- accent
- note length
- master swing
- multiple patterns

## Phase 3 — Project persistence

Goal: make musical work reliably saveable.

Scope:

- project serialization
- pattern/track/step persistence
- migration/version field in project format
- load/save workflow

## Phase 4 — Sampler v0.1

Goal: make imported/factory samples a first-class instrument.

Scope:

- sample import/reference
- audio decoding
- waveform rendering
- Start / End
- One Shot / Gate / Loop
- coarse/fine pitch
- reverse
- root note
- amp envelope
- basic filter

## Phase 5 — Synth v0.1

Goal: create melodic material without external instruments.

Scope:

- two oscillators
- basic waveforms
- noise
- filter
- amp ADSR
- filter ADSR
- LFO
- mono/poly
- glide
- presets
- polyphonic Step notes

## Phase 6 — Advanced step sequencing

Goal: establish the project's deeper sequencer identity.

Candidate order:

1. micro timing
2. probability
3. parameter locks
4. repeat / ratchet
5. conditional triggers
6. track swing override
7. independent track rate

The exact ordering should be revisited after the basic groovebox is playable.

## Phase 7 — Advanced sampling

Candidates:

- sample lock / per-step override
- slicing
- auto slice
- dedicated loop markers
- recording
- multisample
- time stretch

## Phase 8 — Harmony tools

Candidates:

- project key/scale
- chord-name input
- chord-to-notes generation
- inversions
- voicings
- diatonic chord chooser
- voice-leading assistance
- arpeggiator

## Phase 9 — External integration / arrangement

Candidates:

- MIDI input/output
- pattern chaining
- song mode
- export/render
- expanded effects

## Technology decision checkpoint

Before Phase 1 implementation, choose the first concrete stack based on:

- Android/iOS ambitions
- desktop requirements
- audio latency
- DSP extensibility
- waveform rendering
- file import/persistence
- ease of development and debugging

The architecture should permit replacing or augmenting the audio backend later if necessary.
