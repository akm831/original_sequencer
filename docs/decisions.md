# 設計上の決定事項と未決事項

このファイルには、明示的に見直さない限り維持する設計判断を記録します。

## 確定済みの判断

### 製品の位置づけ

本プロジェクトは単なるDrum Machine Cloneではなく、Groovebox型の音楽制作アプリとします。

### 基本操作

Main Sequencing Interactionは、分かりやすい16-step Surfaceを使用します。高度なControlはDetail Viewへ分離します。

### Track Model

TrackがEngineを選択します。初期EngineはSamplerとSynthです。Drum用とMelody用で無関係なSequencer Coreを別々に作りません。

### Step Model

Stepは拡張可能とし、Chord用の複数Noteを含むNote Eventを保持できるようにします。

### VelocityとAccent

VelocityとAccentは別々の音楽的概念として保持します。

### Musical Timing

TimingはMillisecondsだけでなくMusical Unitsで保持します。現在の設計目標は960 PPQNです。

### SwingとMicro Timing

Swingは規則的なGroove Timing、Micro TimingはStep単位のOffsetとして分離します。

### Track Length

Trackごとの独立LengthをCore Capabilityとします。初期範囲は1–16 Stepsです。

### Sampler編集

Sample Start / End、Reverse、Root Note等は原則として非破壊編集にします。

### Synthの方向性

最初のBuilt-in Synthは巨大なThird-party Instrumentを組み込むのではなく、コンパクトなSubtractive Synthesizerとします。

### Chord生成

将来のChord / Harmony機能はAudio Engineより上位に置き、通常のNoteを生成します。特殊なSynth PresetだけでChord機能を実現しません。

### Factory Content

同梱Sample / Presetは再配布可能なLicenseが明確なものだけを使用します。

## 暫定的な判断

以下は現時点での方針ですが、Prototype後に見直す可能性があります。

- 初期Groovebox目標は約8 Tracks
- 初期約8 Patterns
- Factory Samplesは約70–100
- Synth Presetsは約30
- v0.1はMaster Swing、Track Swingは後で追加
- Independent Track Lengthを確認した後にTrack Rateを追加

## 未決事項

### Platform / Framework

未決定です。

これまでの候補:

- 必要に応じNative Audio Integrationを組み合わせたFlutter
- Web Technology + Native / Mobile Wrapper
- 高性能DSP向けNative / JUCE / C++ Audio Layer

単なる実装の手軽さだけでなく、製品要件から決定します。

### Audio Engine

未決事項:

- Fully Shared EngineかPlatform-native Layerか
- Sample-accurate Scheduling Strategy
- Maximum Polyphony
- Voice Stealing
- Resampling / Interpolation Quality
- Filter Implementation
- Effects Architecture

### Project Format

要決定:

- Serialization Format
- Explicit Schema Versioning
- Asset Reference / Portable Project Bundle
- Missing Sample時のBehavior
- Version間Migration

### Step Semantics

詳細未定:

- ProbabilityとConditional Triggerの評価関係
- Repeat / Ratchetの評価順
- Parameter LockのLifetime / Interpolation
- Polyphonic NoteへのParameter Lock
- Sample LockとSampler Parameter Lockの関係

### Pattern Behavior

詳細未定:

- Pattern切替のQuantization
- BPMをPatternごとかGlobalにするか
- Pattern Copy / Duplicate Workflow
- 16を超えるMaximum Pattern Length

### Sampler

詳細未定:

- Exact Loop Behavior
- Start / EndのUnitとPrecision
- One Shot時のEnvelope Behavior
- Slice Representation
- Sample Caching Strategy
- PlatformごとのSupported File Formats

### Synth

詳細未定:

- Initial Polyphony Limit
- Oscillator Anti-aliasing Strategy
- Exact Filter Types
- LFO Sync / Free Modes
- Preset Format

### Harmony

詳細未定:

- Chord MetadataとGenerated Notesのどちらを永続化するか
- Initial Chord Vocabulary
- Scale Model
- Chord Suggestion Behavior
- Voicing Constraints

## Decision Log形式

重要な技術・製品判断を行った場合、以下の形式で追記します。

```text
YYYY-MM-DD — 判断タイトル
決定:
理由:
検討した代替案:
影響:
```

これにより、後のCodex Sessionで背景を失ったまま確定済みArchitectureを再判断することを防ぎます。
