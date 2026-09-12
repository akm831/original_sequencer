# 製品仕様

## 仮の製品定義

本プロジェクトは、高速なステップシーケンサー、Sample再生・編集、Synthesizerを中心としたオリジナルの **グルーヴボックス型音楽制作アプリ** です。

外部音源がなくても、ある程度実用的な音楽スケッチを作れることを目標とします。

## 設計目標

1. **即時性** — 16-stepの操作で素早くBeatを作れること。
2. **必要なときに深い編集** — Main UIは簡潔にしつつ、Step単位の高度な編集を用意すること。
3. **Touch-first** — スマートフォンやTabletでも使いやすく、Mouse/Desktopでも自然に操作できること。
4. **Engine-flexible** — TrackをDrum/Melodyで固定せず、Sound Engineを選択できること。
5. **拡張可能** — Probability、Parameter Lock、Conditional Trigger、Slice、Harmony、MIDI、Song Modeなどを将来追加できること。
6. **非破壊編集** — Sample Start/Endなどは原則として元ファイルを書き換えずPlayback Metadataとして保持すること。

## 参考にする考え方

特定製品のコピーではなく、以下の思想を参考にします。

- TR系の高速なStep入力と視認性
- Elektron系のStep単位の変化とParameter Lock
- 現代的Grooveboxの独立Track Length / Rate
- MPC / Digitakt系の波形中心Sampling
- 必要に応じたDAW的な柔軟な内部Event表現

## 最初の実用版

### Tracks

- 約8 Tracksを目標
- 各TrackでEngineを選択
  - Sampler
  - Synth
- 初期Track Length: 1–16 Steps
- 将来: Trackごとの独立Playback Rate

### Sequencing

- 16-stepを基本Gridとする
- Note / Notes
- Velocity
- Note Length
- Accent
- Master Swing
- 複数Pattern（初期目標: 8）

### Sampler

- 対応可能な環境ではWAV / AIFF等の一般的PCM Audioを読み込み
- Waveform表示
- 非破壊Start / End
- One Shot / Gate / Loop
- Coarse / Fine Pitch
- Reverse
- Root Note Metadata
- Amp Envelope
- Basic Filter

### Synth

初期Synthは減算方式（Subtractive Synthesizer）とします。

- 2 Oscillators
- Sine / Triangle / Saw / Square
- Noise
- Filter
- Amp ADSR
- Filter ADSR
- LFO
- Mono / Poly
- Glide
- Presets

### Factory Content

初期目標:

- 約70–100 Drum / Percussion / FX Samples
- 約30 Synth Presets

同梱素材は、アプリ内での再配布を明確に許可するLicenseのみ使用します。

## 将来機能

以下はv0.1必須ではありませんが、拡張先として設計します。

- Step Probability
- Micro Timing
- Parameter Lock
- Conditional Trigger
- Ratchet / Repeat
- Track Swing
- Independent Track Rate
- Sample Lock / StepごとのSample Override
- Slice / Transient Detection
- Time Stretch
- Multisampling
- Recording
- External MIDI
- FM / Wavetable等の追加Synth Engine
- Project Key / Scale
- Chord Generator
- Inversion / Voicing
- Voice Leading支援
- Arpeggiator
- Pattern Chain / Song Mode
- Mixer FX / Master FX

## 製品階層

```text
Project
├─ Global Musical Settings
├─ Sample Library
├─ Patterns
│  └─ Tracks
│     ├─ Engine
│     ├─ Mixer Settings
│     └─ Steps / Events
└─ Future Song / Arrangement Data
```

## Main Screenの原則

Main Screenは演奏とStep入力に集中させます。Sound Designや詳細Event編集は専用Viewへ分離し、Gridを過密にしません。

## 未決事項

- 最終Target Platforms
- Application Framework
- Native / Shared Audio Engine戦略
- 最大Track数・Polyphony
- 16 Stepsを超えるPattern Length
- 初期ReleaseでのEffects範囲
- 初回公開前にRecordingを必須とするか
