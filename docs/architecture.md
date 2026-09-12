# アーキテクチャ

## 目的

具体的なFrameworkやProgramming Languageを選ぶ前に、責務の境界を定義します。

中心原則は、**音楽的な動作を特定のUI ToolkitやAudio Backendへ密結合させないこと**です。

## 概念レイヤー

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

PersistenceはProject Modelの横に位置し、音楽的挙動を定義せずProject StateとAssetを保存・復元します。

## 主要モジュール

### UI / Interaction

担当:

- Step Grid操作
- Track選択
- Sampler Waveform Editor
- Synth Controls
- Pattern管理
- Transport Controls
- Touch Gestures

UIはModelへIntent / Commandを渡し、Timing-criticalなAudio Scheduling Logicを持たないようにします。

### Project Model

永続化対象となる音楽状態を保持します。

```text
Project
├─ bpm
├─ ppqn
├─ masterSwing
├─ sampleLibrary
├─ patterns[]
└─ future: song/arrangement
```

Project Model自体はFile Pathや特定PlatformのStorage APIへ依存させません。

### Pattern

同時に再生される複数Trackを保持します。

```text
Pattern
├─ id
├─ name
└─ tracks[]
```

### Track

Trackは主要な音楽レーンであり、使用するEngineを選択します。

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

「Drum Track」と「Melody Track」で別々のSequencer Coreを作らず、SamplerとSynthは可能な限り同じSequencing Coreを共有します。

### Transport / Clock

担当:

- Play / Stop
- BPM
- Musical Position
- Step / Bar Boundary
- Musical TimeからScheduled Audio Timeへの変換
- Lookahead Scheduling

現在のTiming設計目標:

- 960 PPQN
- Timingはミリ秒だけでなくMusical Units / Ticksで保持

```text
scheduled musical time
= grid position
+ swing offset
+ micro timing offset
```

最終的なSeconds / Audio Framesへの変換はAudio Backendに近い層で行います。

### Engine Abstraction

TrackがSound Engineを選択します。初期Engine:

- Sampler
- Synth

将来候補:

- FM Synth
- Wavetable Synth
- External MIDI

SequencerはMusical EventとParameter Overrideを記述し、Engine内部の発音方式には依存しません。

### Mixer / FX

Trackごとの基本概念:

- Level
- Pan
- Mute
- Solo

将来:

- Sends
- Insert Effects
- Master Effects

### Persistence

担当:

- Project MetadataのSerialization / Deserialization
- Explicit Schema Versioning
- Schema Migration
- Sample Asset管理
- Missing Asset検出 / Relink
- Save Validation
- Atomic Save相当の安全な書込

Project MetadataはJSON互換の明示的Schemaを初期候補とします。

SampleはOSの絶対Pathを正本にせず、Project内のStable Asset IDで参照します。

```text
Sampler State
└─ sampleAssetId

Sample Library
└─ SampleAsset
   ├─ id
   ├─ relativePath
   ├─ fileName
   └─ optional metadata
```

Imported Sampleは原則としてPortable Project Assetへ取り込みます。

PresetはPreset名だけを保存せず、Project保存時点の実際のEngine Stateを保存します。

詳細は`docs/persistence.md`を参照します。

## Event Flow

Synth Noteの例:

```text
Step到達
→ Probability / Condition評価（実装後）
→ Note Event生成
→ Parameter Lock適用
→ SynthへNote + Parameters送信
→ Mixer
→ Output
```

Sampler Triggerの例:

```text
Step到達
→ SequencerがStep評価
→ SamplerへNote / Trigger送信
→ Sample選択・Start/End・Pitch解決
→ Voice再生
→ Mixer
→ Output
```

## Parameter Model

将来のParameter Lockのため、安定したParameter IDを使用します。

```text
sampler.start
sampler.pitch.coarse
sampler.filter.cutoff
synth.osc1.wave
synth.filter.cutoff
synth.lfo.amount
```

StepにはOverrideされたParameterだけを保存し、通常値はTrack / Engine Stateが保持します。

## 非破壊Audio File方針

Start / End、Loop Marker、Reverse、Root Note等は通常Project Metadataとして保存し、元のAudio Fileを変更しません。

Crop / Normalizeなどの破壊的処理を将来追加する場合は、明示的な別操作にします。

## Platform戦略

現時点ではPlatformを固定しません。Application / Sequencer Layerを共有しつつ、LatencyやDSP要件に応じてAudio BackendだけNative化できる構造を維持します。

Storage UIやProject Bundleの実装はPlatformごとに異なって構いませんが、Project SchemaとStable Asset Referenceの意味は共通にします。

具体的な技術選定はCore Musical Modelが十分固まってから行います。
