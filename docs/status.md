# Project Status

このファイルは、新しいChatGPT / Codex Sessionで現在地を短時間で復元するための「しおり」です。詳細仕様の正本は各`docs/*.md`、判断理由は`docs/decisions.md`です。

## Phase

Specification / Architecture → Technology Prototype

製品本実装（Phase 1）はまだ開始していません。Flutter UI + Native / C++ AudioとJUCE / C++を共通Prototypeで比較してからTechnologyを決定します。

## Product Direction

Touch-first Groovebox / Sequencer。初期約8 Tracks、各TrackがSamplerまたはSynth Engineを選択。Android第一ターゲット、Web第二ターゲット。Core Musical LogicはPlatform固有APIへ依存させない。

## Recently Completed

- 960 PPQN Timing、Sample-accurate Scheduling、Audio Thread責務など主要仕様を定義
- Framework非依存C++20 Common Reference Audio Coreとhost smoke tests
- Flutter Dart FFI / Android native bridge / Gradle / CMake / Oboe 1.10.0 wiring
- Flutter Android P1実機成功: 48000 Hz、96 frames/callback、Rendered Frames継続増加、Restart Count 0、220 Hz test tone実発音
- JUCE Android P1実機成功: 48000 Hz、96 frames/callback、Restart Count 0、220 Hz test tone実発音
- Flutter / Oboeへ`ErrorDisconnected`検出とstream再openの最小Device Restart経路を追加
- Device Restart対応追加後も通常の220 Hz Audio出力が正常であることを確認
- Flutter P2 Callback Timing / Audio Frame Timeline / Callback Frames min-max / Load Histogramを実装
- Histogram解像度を0.1 percentage pointへ修正し、sub-1%負荷のPercentileを観測可能にした
- Flutter P2 Reference Device baseline確定: 48000 Hz / callbackFrames 96 (min 96 / max 96) / current load約0.70% / p95 0.7% / p99 0.7% / peak 33.80%

## Current Topic

Technology Prototype: Bounded Command Queue → Sample-accurate Trigger

現在の到達点:

- JUCE / Flutter双方でAndroid P1基本Bring-up確認済み
- Flutter P2 Callback Load / Percentile / Audio Frame Timeline baseline確認済み
- 48000 Hz / 96 framesではcallback budget約2000 us。通常負荷約0.7%で十分な余裕がある
- p95 / p99とも0.7%で、約34%のPeakは通常負荷ではなく稀なSpikeとして扱う
- Flutter Device Restartコードはbuild・通常再生確認済み。route change実機試験は機材準備後に行う
- JUCE Device RestartとP2同等計測は未確認

次に進める主題:

1. Common Audio Coreへ固定容量SPSC Bounded Command Queueを追加する
2. Queue Depth / High Water Mark / Overflow CountをDiagnosticsへ追加する
3. Audio Thread側でtimestamp付きCommandをallocation / lockなしでconsumeする
4. 同一Callback内の異なるSample OffsetでTriggerできる最小Vertical Sliceをhost testで確認する
5. Flutter FFIから最小Trigger Commandを投入し、実機でSample-accurate Trigger経路を確認する
6. 同じ構造をJUCE Candidateでも使用し比較する

## Important Current Decisions

- SequencerとAudio Engineを分離する
- Audio ThreadはUI / Project Model / File I/Oへ直接依存しない
- SchedulerとAudio Callbackを分離し、LookaheadでTimestamp付きAudio Commandを準備する
- Buffer内EventはSample Offset位置で実行する
- Audio Logicを特定Buffer Sizeへ固定しない
- 128 Frames程度をPreferred Target、256 Frames程度をStable Fallbackとする
- Callback Loadは単発PeakだけでなくPercentileと継続負荷で評価する
- Audio callback内でallocation / lock / loggingを行わない
- Command QueueはまずSingle Producer / Single Consumerの固定容量構造で検証する
- Queue overflowは待機せず失敗として記録し、Audio Threadをblockしない
- Framework非依存C++ Reference Audio Coreを候補間で共有する
- Android第一ターゲット、Web第二ターゲット
- Device Restartでは旧Audio Frame Timelineを継続しない
- 最終Technology Decision前にiOSでもMust要件Smoke Testを行う

## Primary References

1. `AGENTS.md`
2. `docs/status.md`
3. `docs/technology-prototype.md`
4. `docs/prototype-implementation-plan.md`
5. `docs/prototype-build-notes.md`
6. `docs/framework-comparison.md`
7. `docs/platform-audio-requirements.md`
8. `docs/performance-budget.md`
9. `docs/audio-buffer-latency.md`
10. `docs/audio-engine.md`
11. `docs/architecture.md`
12. `docs/decisions.md`
13. `docs/roadmap.md`

## Session Handoff Policy

Session終了前にRepositoryを更新し、新しいSessionではGitHubをSource of Truthとして復元します。

## Next

Common Audio Coreへ最小SPSC Bounded Command Queueを追加する。最初はtimestamp付きTrigger CommandとQueue Diagnosticsに限定し、host smoke testでcapacity / FIFO / overflow / callback境界を検証してからFlutter FFIへ接続する。Device Restart route change試験は機材準備後に戻って実施する。
