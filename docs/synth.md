# Synthesizer Specification

## Goal

Provide a built-in instrument capable of basses, leads, plucks, pads, simple chords, and effects without external plugins.

The first synth engine should be deliberately compact and understandable: a subtractive synthesizer rather than a large multi-engine instrument.

## Initial architecture

```text
OSC 1 ─┐
       ├─ Mixer ─ Filter ─ Amp ─ Output
OSC 2 ─┤
Noise ─┘

Filter Envelope → Filter
Amp Envelope    → Amp
LFO             → selectable destination(s)
```

## Oscillators

Each oscillator should initially support:

- Sine
- Triangle
- Saw
- Square

Parameters:

- wave
- octave
- coarse/fine tune as needed
- level

OSC 2 should support detuning against OSC 1.

## Noise

Simple noise source with level control.

## Filter

Initial controls:

- cutoff
- resonance
- envelope amount

Initial filter type can be Low Pass; additional types may follow.

Where practical, Sampler and Synth should share compatible filter concepts/implementation.

## Envelopes

### Amp Envelope

ADSR:

- Attack
- Decay
- Sustain
- Release

### Filter Envelope

ADSR plus envelope amount.

## LFO

Initial controls:

- rate
- amount
- destination

Possible initial destinations:

- pitch
- filter cutoff
- amplitude

More sophisticated modulation routing can be deferred.

## Voice mode

- Mono
- Poly
- Glide/Portamento for mono behavior

Exact polyphony limit remains an implementation decision and should be recorded when chosen.

## Sequencer integration

Synth tracks consume the same Step model as sampler tracks.

Examples:

```text
notes = [C2]
velocity = 110
length = 0.75 step
```

Chord example:

```text
notes = [C3, E3, G3, B3]
```

## Parameter Lock integration

Future P-Lock targets should include musically useful parameters such as:

- oscillator wave
- oscillator tune/detune
- oscillator levels
- filter cutoff
- filter resonance
- filter envelope amount
- envelope times
- LFO rate
- LFO amount
- glide

Stable parameter identifiers should be used, for example:

```text
synth.osc1.wave
synth.osc2.detune
synth.filter.cutoff
synth.filter.resonance
synth.lfo.rate
synth.lfo.amount
```

## Presets

Initial target: approximately 30 useful presets across categories such as:

- Bass
- Lead
- Pad
- Pluck
- Chord
- FX

Preset files/state should describe synth parameters; chord identity itself should not be baked into a preset because chord generation belongs to the Harmony/Sequencer layer.

## Future synth engines

Potential additions:

- FM
- Wavetable
- more advanced modulation matrix

These should appear as additional engine types or engine modes without requiring a rewrite of the sequencer.
