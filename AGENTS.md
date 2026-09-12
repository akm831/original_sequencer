# AGENTS.md

このリポジトリは、仕様を先に固めてから実装する音楽ソフトウェアプロジェクトです。

## 現在の段階

現在は製品動作とアーキテクチャを定義している段階です。後の決定文書で明示されるまでは、Framework、Language、UI Toolkit、Audio Backendが確定済みだと仮定しないでください。

## 仕様の正本

実装前に必ず次を確認してください。

1. `docs/product.md`
2. `docs/architecture.md`
3. `docs/sequencer.md`
4. `docs/pattern.md`
5. `docs/persistence.md`
6. 対象Engineの仕様（`docs/sampler.md`、`docs/synth.md`、`docs/harmony.md`）
7. `docs/decisions.md`
8. `docs/roadmap.md`

コードと仕様が矛盾する場合、黙ってどちらかを選ばないでください。原則として文書化された仕様を優先し、設計を意図的に変更する場合は仕様書も更新してください。

## アーキテクチャ上の制約

次の責務を分離してください。

- UI / Interaction
- Transport / Musical Clock
- Sequencer Model
- Track / Pattern / Project Model
- Sound Engine Abstraction
- Sampler Engine
- Synth Engine
- Mixer / Effects
- Persistence

Sequencerは特定のUI FrameworkやAudio Backendへ直接依存させないでください。

Trackは「ドラムTrack」「メロディTrack」と固定するのではなく、使用するAudio Engineを選択する構造にします。

Stepのデータ構造は、将来のProbability、Micro Timing、Conditional Trigger、Repeat / Ratchet、Sample Lock、Parameter Lockを追加できるよう拡張可能にしてください。

音楽的なTimingは、ミリ秒だけで保存せず、高解像度の音楽時間表現を使用します。現時点の設計目標は960 PPQNです。

PersistenceではProject ModelとFile Format実装を分離し、ProjectをPlatform固有の絶対Pathだけへ依存させないでください。保存Schemaには明示的なVersionを持たせ、SampleはStable Asset IDで参照します。

## 実装スタイル

小さく検証可能な単位で進めてください。広範囲な骨組みだけを先に作るより、実際に演奏・検証できるVertical Sliceを優先します。

推奨実装順序：

1. Project Skeleton
2. Transport / Musical Clock
3. 1 Trackの16-step Sequencer
4. 1つのSound Source
5. Multiple Tracks
6. Track Length
7. Persistence
8. Sampler Waveform / Editor
9. Synth Engine
10. Advanced Sequencing

将来役立ちそうという理由だけで高度な抽象化を先に追加しないでください。データモデルには拡張余地を残しつつ、初期実装は理解しやすく保ちます。

## 製品原則

- Touch-first UI
- すぐに音楽制作を始められること
- Main Screenは簡潔にし、詳細編集は別Viewへ分離
- Sample編集は原則として非破壊
- SamplerとSynthは可能な限り同じSequencer semanticsを共有
- ChordはSynth Presetに焼き込まず、通常のNote Eventとして表現
- Factory Contentは再配布可能なLicenseのみ使用
- Imported SampleはPortable Project Assetとして扱う
- Preset名だけでなく実際のEngine StateをProjectへ保存する

## 設計判断を行うとき

次のような重要な判断は `docs/decisions.md` に記録してください。

- Framework / Language
- Audio Backend
- Supported Platforms
- Persistence Format
- Timing Model
- Polyphony Limit
- Parameter Automation / Parameter Lock表現
- Sample File Handling

## Coding Agentの作業ルール

機能実装を依頼された場合：

- まず関連仕様を確認する
- 重要な仮定を明示する
- 無関係なRefactorを避ける
- 可能な範囲でTestを追加・更新する
- 変更単位を小さく保つ
- ユーザーに見える動作を変えた場合はDocumentationも更新する
