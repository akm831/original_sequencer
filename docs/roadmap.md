# Roadmap

このRoadmapは、早い段階から実際に演奏・検証できるVertical Sliceを作り、大きすぎて検証不能な構造を先に作らないため、段階的に進めます。

## Phase 0 — 仕様策定

Status: 現在のPhase。

成果物:

- 製品定義
- Conceptual Architecture
- Sequencer Model
- Sampler Requirements
- Synth Requirements
- Harmonyの将来要件
- Technology / Platform判断は後で行う

完了条件:

- Prototypeを明確な期待値に照らして評価できる程度までBehaviorが定義されていること

## Phase 1 — 最小Playable Sequencer

目的: TimingとInteractionを検証する。

範囲:

- Application Skeleton
- Transport: Play / Stop
- BPM
- 16-step Grid
- 1 Track
- 1つのSimple Sound Source
- Reliable Lookahead Scheduling
- Visual Playhead

高度なSound DesignやPersistenceはまだ不要です。

## Phase 2 — Core Groovebox Structure

目的: Track / Pattern Modelを検証する。

範囲:

- Multiple Tracks（約8へ拡張）
- Track Mute / Solo / Level
- Independent Track Length 1–16
- Velocity
- Accent
- Note Length
- Master Swing
- Multiple Patterns

## Phase 3 — Project Persistence

目的: 制作した内容を確実に保存・復元できるようにする。

範囲:

- Project Serialization
- Pattern / Track / Step Persistence
- Project FormatのMigration / Version Field
- Load / Save Workflow

## Phase 4 — Sampler v0.1

目的: Imported / Factory Sampleを主要Instrumentとして利用可能にする。

範囲:

- Sample Import / Reference
- Audio Decoding
- Waveform Rendering
- Start / End
- One Shot / Gate / Loop
- Coarse / Fine Pitch
- Reverse
- Root Note
- Amp Envelope
- Basic Filter

## Phase 5 — Synth v0.1

目的: 外部InstrumentなしでMelodic Materialを作れるようにする。

範囲:

- Two Oscillators
- Basic Waveforms
- Noise
- Filter
- Amp ADSR
- Filter ADSR
- LFO
- Mono / Poly
- Glide
- Presets
- Polyphonic Step Notes

## Phase 6 — Advanced Step Sequencing

目的: 本プロジェクト独自の深いSequencer Identityを確立する。

候補順序:

1. Micro Timing
2. Probability
3. Parameter Locks
4. Repeat / Ratchet
5. Conditional Triggers
6. Track Swing Override
7. Independent Track Rate

Basic GrooveboxがPlayableになった段階で順序を再評価します。

## Phase 7 — Advanced Sampling

候補:

- Sample Lock / Per-step Override
- Slicing
- Auto Slice
- Dedicated Loop Markers
- Recording
- Multisample
- Time Stretch

## Phase 8 — Harmony Tools

候補:

- Project Key / Scale
- Chord-name Input
- Chord-to-notes Generation
- Inversions
- Voicings
- Diatonic Chord Chooser
- Voice-leading Assistance
- Arpeggiator

## Phase 9 — External Integration / Arrangement

候補:

- MIDI Input / Output
- Pattern Chaining
- Song Mode
- Export / Render
- Expanded Effects

## Technology Decision Checkpoint

Phase 1の実装前に、以下を基準として最初の具体的Stackを選択します。

- Android / iOSへの展開意向
- Desktop要件
- Audio Latency
- DSP Extensibility
- Waveform Rendering
- File Import / Persistence
- Development / Debuggingのしやすさ

必要であれば後からAudio Backendを置き換え・拡張できるArchitectureを維持します。
