# Original Sequencer

高速なステップシーケンサー、サンプラー、シンセサイザーを中心とした、グルーヴボックス型の音楽制作アプリです。

現在は **仕様策定・アーキテクチャ設計段階** です。実装技術や対象プラットフォームは、現時点では意図的に固定していません。

## 製品の方向性

中心となる考え方は次のとおりです。

- TR系の即時性と、分かりやすい16ステップの操作感
- Elektron系のParameter LockやProbabilityのようなステップ単位の深い編集
- 現代的なグルーヴボックスに見られるトラックごとのLength / Rate
- 波形編集を備えた内蔵Sampler
- メロディやコードを作れる内蔵Synth
- 将来のコード生成・スケール支援を行うHarmony Engine
- モバイルにも展開しやすいTouch-firstな操作設計

## 初期製品目標

最初の実用版は、単なるドラムマシンではなく、小型グルーヴボックスとして成立させます。

- 約8 Tracks
- TrackごとにSampler / Synthを選択
- 16-step Sequencer
- Trackごとの独立Length
- Velocity / Accent / Note Length / Swing
- 波形を使ったSample編集
- 基本的な減算方式Synth
- 複数Pattern
- Factory Samples / Synth Presets

Probability、Micro Timing、Parameter Lock、Conditional Trigger、Slice、Song Mode、MIDIなどの高度な機能は、最初の実装段階で必須とはせず、将来拡張できる構造として設計します。

## ドキュメント

- [`AGENTS.md`](AGENTS.md) — Codex / coding agent向け作業指針
- [`docs/product.md`](docs/product.md) — 製品ビジョンとスコープ
- [`docs/architecture.md`](docs/architecture.md) — 概念アーキテクチャ
- [`docs/sequencer.md`](docs/sequencer.md) — Sequencerのデータモデルと動作
- [`docs/sampler.md`](docs/sampler.md) — Sampler仕様
- [`docs/synth.md`](docs/synth.md) — Synthesizer仕様
- [`docs/harmony.md`](docs/harmony.md) — 将来のChord / Harmony Engine仕様
- [`docs/roadmap.md`](docs/roadmap.md) — 段階的な実装計画
- [`docs/decisions.md`](docs/decisions.md) — 設計上の決定事項と未決事項

## 開発方針

最初から大規模なアプリ全体を一度に作らず、早い段階から実際に鳴らして検証できる小さな縦方向の実装（vertical slice）を積み重ねます。

`docs/` 以下の仕様を製品動作の正本（source of truth）とします。実装上の都合で仕様と異なる動作へ変更する場合は、暗黙に変更せず、関連する仕様書も意図的に更新します。
