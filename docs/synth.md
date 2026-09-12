# Synthesizer仕様

## 目的

外部PluginなしでもBass、Lead、Pluck、Pad、Chord、FXを作れる内蔵音源を提供します。

最初のSynth Engineは巨大なMulti-engine Instrumentではなく、Step Sequencerとの連携を重視した、理解しやすいコンパクトな減算方式Synthesizerとします。

重要なのは機能数ではなく、少ないParameterで音楽的な変化を作りやすく、Parameter LockやAutomationへ自然に拡張できることです。

## v0.1の基本方針

- 2 Oscillators + Noise
- Sub Oscillatorは初期版では追加しない
- LP / HP Filter
- Amp ADSR
- Filter ADSR
- 1系統のLFO
- Mono / Poly
- Mono時のGlide
- 1 Synth Trackあたり最大8 Voiceを暫定目標とする
- Chord生成はSynth内部ではなくHarmony / Sequencer Layerで行う

## Signal Flow

```text
OSC 1 ─┐
       ├─ Mixer ─ Filter ─ Amp ─ Output
OSC 2 ─┤
Noise ─┘

Filter Envelope → Filter
Amp Envelope    → Amp
LFO             → selected destination
```

## Oscillators

各Oscillatorは初期段階で以下のWaveformに対応します。

- Sine
- Triangle
- Saw
- Square

各OSCのParameter:

- Wave
- Octave
- Coarse Tune
- Fine Tune
- Level

OSC 2はOSC 1に対してDetuneしやすいUIを用意します。

### 初期版で入れないもの

- Sub Oscillator
- Hard Sync
- Ring Modulation
- FM between oscillators
- Wavetable

これらは音作りの幅を広げますが、v0.1ではUIとDSPの複雑さを増やすため後回しにします。

## Noise

Simple Noise Sourceを1系統用意します。

Parameter:

- Level

Noise Colorや複数Noise Typeは将来拡張とします。

## Filter

v0.1 Filter Type:

- Low Pass (LP)
- High Pass (HP)

将来候補:

- Band Pass (BP)
- additional character / drive modes

Parameter:

- Type
- Cutoff
- Resonance
- Envelope Amount

可能であればSamplerとSynthで互換性のあるFilter概念・Parameter Namingを共有します。

Filter CutoffはParameter Lock対象として特に重要なため、リアルタイム変更時に不自然なClickや破綻が出にくい実装を優先します。

## Envelopes

### Amp Envelope

ADSR:

- Attack
- Decay
- Sustain
- Release

音量の時間変化を制御します。

### Filter Envelope

ADSR:

- Attack
- Decay
- Sustain
- Release
- Envelope Amount

Amp EnvelopeとFilter Envelopeは別々に保持します。

## LFO

v0.1ではLFOを1系統とします。

Parameter:

- Mode
  - Free
  - Tempo Sync
- Rate
- Amount
- Destination

初期Destination:

- Pitch
- Filter Cutoff
- Amplitude

Tempo Sync時はBPMに追従する音楽的Divisionを選択します。

例:

```text
1/1
1/2
1/4
1/8
1/16
```

Triplet / Dotted Divisionや複数LFO、高度なModulation Matrixは後回しにします。

v0.1では1つのLFOにつき1 Destinationで十分とします。

## Voice Mode

### Mono

主にBass / Lead向けです。

- 1 Voice
- Glide / Portamento対応

### Poly

Chord / Pad / Pluck向けです。

暫定設計目標:

- 1 Synth Trackあたり最大8 Voice

8 Voiceとする理由:

- 3～4音Chordを十分扱える
- Release中の前のChordと次のChordが一時的に重なれる
- Mobileを含む将来Platformで負荷を制御しやすい

最終的なGlobal Polyphony Limit、Voice Stealing Ruleは実装Prototypeで検証して決定します。

## Glide / Portamento

GlideはMono Modeを中心に使用します。

Parameter:

- Glide Time

将来的にAlways / Legato OnlyなどのModeを追加可能ですが、v0.1では単純な仕様から開始します。

## Velocityとの関係

Velocityは少なくともAmp Levelへ反映できるようにします。

将来的にはVelocity Sensitivityとして以下へ拡張可能です。

- Amp
- Filter Cutoff
- Filter Envelope Amount

ただしv0.1では複雑なVelocity Routingは不要です。

## Sequencer連携

Synth TrackはSampler Trackと同じStep Modelを使用します。

単音例:

```text
notes = [C2]
velocity = 110
length = 0.75 step
```

Chord例:

```text
notes = [C3, E3, G3, B3]
```

Sequencer側がNote / Chord / Length / Velocityを管理し、Synthは渡されたNote Eventを発音します。

### Harmony Engineとの関係

Chord名からNoteへ展開する処理はSynth内部に持たせません。

```text
Cm7
↓
Harmony Engine
↓
[C3, Eb3, G3, Bb3]
↓
Sequencer Note Event
↓
Synth Engine
```

この構造により、同じHarmony機能を将来Pitched SamplerやExternal MIDIにも利用できます。

## Parameter Lock連携

SynthはParameter Lockと強く連携できるよう設計します。

### 優先度が高いP-Lock対象

- Filter Cutoff
- Filter Resonance
- Filter Envelope Amount
- OSC 1 / OSC 2 Tune
- OSC 2 Detune
- OSC Levels
- Noise Level
- Envelope Times
- LFO Rate
- LFO Amount
- Glide Time

### 将来または慎重に扱う対象

- Oscillator Wave switching
- Filter Type switching
- Voice Mode switching

DiscreteなType変更はStep境界で音切れやClickを起こす可能性があるため、初期P-LockではContinuous Parameterを優先します。

安定したParameter IDを使用します。

```text
synth.osc1.wave
synth.osc1.level
synth.osc2.detune
synth.filter.type
synth.filter.cutoff
synth.filter.resonance
synth.filter.envAmount
synth.lfo.mode
synth.lfo.rate
synth.lfo.amount
synth.glide.time
```

Parameter IDはProject Save後やVersion Update後も可能な限り互換性を保ちます。

## Smartphone UI方針

Main Step GridへSynth Parameterを大量に置きません。

Trackを選択してSynth Editorを開き、少数のPageへ分割します。

推奨構成:

```text
SYNTH
├─ OSC
├─ FILTER
├─ ENV
└─ MOD / VOICE
```

### OSC Page

- OSC1 Wave / Octave / Tune / Level
- OSC2 Wave / Octave / Tune / Detune / Level
- Noise Level

### FILTER Page

- Filter Type
- Cutoff
- Resonance
- Envelope Amount

### ENV Page

- Amp ADSR
- Filter ADSR

### MOD / VOICE Page

- LFO Mode / Rate / Amount / Destination
- Mono / Poly
- Glide

1画面にすべてを表示するより、Touch Targetを十分大きく取ることを優先します。

## Presets

初期目標は約30 Presetsです。

例:

- Bass
- Lead
- Pad
- Pluck
- Chord
- FX

PresetはSynth Parameter Stateを保存します。

ChordそのものはPresetへ焼き込まず、Harmony / Sequencer Layerで生成します。

Preset Formatには将来の互換性のためVersion情報を持たせる方針とします。

## v0.1で意図的に後回しにするもの

- Sub Oscillator
- multiple LFOs
- Modulation Matrix
- FM
- Wavetable
- oscillator sync
- ring modulation
- built-in arpeggiator inside Synth
- complex unison / supersaw
- per-voice modulation routing

必要になった場合も、Sequencer Coreを書き直さずSynth Engine内部の拡張として追加できる構造を維持します。

## 将来のSynth Engine

候補:

- FM
- Wavetable
- より高度なSubtractive Synth
- 高度なModulation Matrix

これらは追加Engine TypeまたはEngine Modeとして拡張し、Sequencerからは共通のNote Event / Parameter Interfaceを利用します。
