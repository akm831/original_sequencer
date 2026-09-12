# Synthesizer仕様

## 目的

外部PluginなしでもBass、Lead、Pluck、Pad、簡単なChord、FXを作れる内蔵音源を提供します。

最初のSynth Engineは巨大なMulti-engine Instrumentではなく、理解しやすいコンパクトな減算方式Synthesizerとします。

## 初期アーキテクチャ

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

各Oscillatorは初期段階で以下を対応します。

- Sine
- Triangle
- Saw
- Square

Parameters:

- Wave
- Octave
- 必要に応じてCoarse / Fine Tune
- Level

OSC 2はOSC 1に対するDetuneを設定できるようにします。

## Noise

Level Control付きのSimple Noise Sourceを用意します。

## Filter

初期Control:

- Cutoff
- Resonance
- Envelope Amount

初期Filter TypeはLow Passで構いません。その他のTypeは将来追加可能にします。

可能であればSamplerとSynthで互換性のあるFilter概念・実装を共有します。

## Envelopes

### Amp Envelope

ADSR:

- Attack
- Decay
- Sustain
- Release

### Filter Envelope

ADSR + Envelope Amount。

## LFO

初期Control:

- Rate
- Amount
- Destination

初期Destination候補:

- Pitch
- Filter Cutoff
- Amplitude

高度なModulation Routingは後回しにします。

## Voice Mode

- Mono
- Poly
- Mono時のGlide / Portamento

具体的なPolyphony Limitは実装時に決定し、`docs/decisions.md` に記録します。

## Sequencer連携

Synth TrackはSampler Trackと同じStep Modelを使用します。

```text
notes = [C2]
velocity = 110
length = 0.75 step
```

Chord例:

```text
notes = [C3, E3, G3, B3]
```

## Parameter Lock連携

将来のP-Lock対象候補:

- Oscillator Wave
- Oscillator Tune / Detune
- Oscillator Levels
- Filter Cutoff
- Filter Resonance
- Filter Envelope Amount
- Envelope Times
- LFO Rate
- LFO Amount
- Glide

安定したParameter IDを使用します。

```text
synth.osc1.wave
synth.osc2.detune
synth.filter.cutoff
synth.filter.resonance
synth.lfo.rate
synth.lfo.amount
```

## Presets

初期目標は約30 Presetsです。

- Bass
- Lead
- Pad
- Pluck
- Chord
- FX

PresetはSynth Parameter Stateを保存します。ChordそのものはPresetへ焼き込まず、Harmony / Sequencer Layerで生成します。

## 将来のSynth Engine

候補:

- FM
- Wavetable
- 高度なModulation Matrix

Sequencerを書き直さず、追加Engine TypeまたはEngine Modeとして拡張できる構造にします。
