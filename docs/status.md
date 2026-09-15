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
- Device Restart対応追加後もReference Deviceで通常の220 Hz Audio出力が正常であることを確認
- Flutter P2 Callback Timing DiagnosticsをCommon Core → FFI → Dart UIへ追加
- Flutter P2実機確認: callbackDurationUsは通常約13 us、軽いcallbackでは約4 us、callbackLoadは通常約0.6%、観測時callbackLoadPeakは33.98%
- Flutter P2 Audio Frame Timeline (`callbackStartFrame`) とRendered Framesが連続して進む構造を実装

## Current Topic

Technology Prototype: P2 Realtime Diagnostics → Bounded Command Queue / Sample-accurate Trigger

現在の到達点:

- JUCE / Flutter双方でAndroid P1基本Bring-up確認済み
- Flutter P2 Callback Duration / Current Load / Peak / Audio Frame Timelineを実機観測済み
- 48000 Hz / 96 framesではcallback budgetは約2000 us。通常約13 us ≒ 0.65%で、表示値約0.6%と整合
- callbackLoadPeak 33.98%は単発Peakだけでは評価せず、Percentile計測を追加して頻度を判定する
- Flutter Device Restartコードはbuild・通常再生確認済み。Headphone / USB route change実機試験は機材準備後に行う
- JUCE Device RestartとP2同等計測は未確認

次に進める主題:

1. Callback Load Percentile（まずp95/p99相当）をRealtime安全な固定サイズ集計で追加する
2. Callback Framesの変動を観測できるDiagnosticsを追加し、特定Buffer Sizeへの依存がないことを確認する
3. P2結果をBaselineとして固定する
4. Bounded Command QueueをCommon Core側へ追加する
5. 同一Callback内の異なるSample OffsetでTriggerできる最小Vertical Sliceへ進む
6. FlutterとJUCEで同じStress条件を比較する

## Important Current Decisions

- SequencerとAudio Engineを分離する
- Audio ThreadはUI / Project Model / File I/Oへ直接依存しない
- SchedulerとAudio Callbackを分離し、LookaheadでTimestamp付きAudio Commandを準備する
- Buffer内EventはSample Offset位置で実行する
- Audio Logicを特定Buffer Sizeへ固定しない
- 128 Frames程度をPreferred Target、256 Frames程度をStable Fallbackとする
- Callback Loadは単発PeakだけでなくPercentileと継続負荷で評価する
- Diagnostics集計のためにAudio Callback内でallocation / lock / loggingを行わない
- Framework非依存C++ Reference Audio Coreを候補間で共有する
- Android第一ターゲット、Web第二ターゲット
- Device Restartでは旧Audio Frame Timelineを継続しない
- Audio callback自身からstream close/reopenを行わない
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

Flutter P2へRealtime安全なCallback Load PercentileとCallback Frames変動Diagnosticsを追加する。Audio callback内では固定サイズ/atomic中心の処理に留め、allocation・mutex・loggingを避ける。実機で通常Load、p95/p99、Peak、callback frame rangeを確認後、Bounded Command Queue / Sample-accurate Triggerへ進む。Device Restart route change試験は機材準備後に戻って実施する。
