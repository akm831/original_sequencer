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

- Common Coreへ256容量SPSC Queue、timestamp付きCommand、Queue / Trigger Diagnosticsを実装済み（`9b2f873`まで。旧statusには未反映だった）
- 2026-10-01: Diagnosticsのpending flagをatomic化し、Queue Depthの並行読み取りを容量範囲へ制限
- CallbackのCommand処理を最大257件に制限し、Producerの連続投入で処理が無限に延びることを防止
- Host smoke tests 3本が成功。10万件の並行FIFO転送、Callback境界、可変Buffer、Restart時Pending破棄を確認

- 2026-10-02: Common Coreへ指定Sample Offsetで始まる50 ms / 220 Hz検証用Burstを実装。Callback分割前後で波形が一致するHost testを追加
- Flutter C ABI / DartへScheduled Trigger APIとQueue / Trigger Diagnosticsを接続し、P3検証ボタンを追加
- Android AdapterでCommand投入とCore LifecycleをControl側Mutexにより直列化（Audio CallbackはMutex不使用）
- Host tests 4本、UndefinedBehaviorSanitizer / ThreadSanitizer、Oboe 1.10.0ヘッダーによるAndroid C++構文検査が成功。その後GitHub ActionsでFlutter / NDK build成功。P3実機発音は2026-10-03にユーザー確認

- JUCEへ同じScheduled Trigger入力・P2 Timing / Percentile計測・Queue表示を接続。CoreへPlanar出力を追加し、Android Interleaved出力との波形一致をHost検証
- JUCE 9.0.2公式HeaderでMain.cppの構文検査成功。JUCEのLink / Android実build・実機確認は未実施

- Flutter候補の自動Build Workflowを追加: C++ tests → Flutter analyze → ARM64 Debug APK → Native Library Packaging検査 → Artifact保存。初回Run `36962438772`ですべて成功

## Current Topic

Technology Prototype: P4 UI整理 / 再生安定性

現在の到達点:

- JUCE / Flutter双方でAndroid P1基本Bring-up確認済み
- Flutter P2 Callback Load / Percentile / Audio Frame Timeline baseline確認済み
- 48000 Hz / 96 framesではcallback budget約2000 us。通常負荷約0.7%で十分な余裕がある
- p95 / p99とも0.7%で、約34%のPeakは通常負荷ではなく稀なSpikeとして扱う
- Flutter Device Restartコードはbuild・通常再生確認済み。route change実機試験は機材準備後に行う
- JUCE Device RestartとP2同等計測は未確認

実装と確認の区別:

- Common Coreは指定FrameからOffsetを計算し、その位置から検証用Burstを実際に出力する。Host testで発音位置・Buffer境界・可変Buffer・Release後silenceを確認済み
- Flutterからの投入とDiagnostics表示は実装済み。従来の連続Test Toneを止め、通常は無音、ボタンで短いBurstを出す
- P3の検証音はMonophonic / Retrigger方式。製品版Synth / Poly Voice実装ではなく、Live Padの低Latency経路とも区別する
- GitHub ActionsでCMakeによるHost tests 4本、Flutter analyze、ARM64 Android Debug APK build、必要Native LibrariesのPackaging検査が成功。実機P3はユーザーがボタン押下で発音を確認済み（2026-10-03）
- JUCEのP3 UI・Burst出力・P2同等計測も実装済み。両候補とも連続Test Toneを止め、同じCoreの検証用Burstを出力。実機比較はまだ完了していない

P4の実装:

- Flutterに再生／停止、60〜240 BPM、16ステップのオン／オフ、再生位置表示を追加。初期120 BPM、1 / 5 / 9 / 13ステップ有効
- 共通C++ Schedulerは960 PPQN、1/16 = 240 ticks、約50 ms Lookahead。AndroidのNative Control Threadが5 ms間隔で準備し、UI Timerは表示更新のみ
- Stop / Playの世代番号で旧Commandを破棄。停止後の既存Burstは最大50 msで終了。再生は常にステップ1から再開
- BPM / Pattern変更は未予約のEventへ適用。BPMは次の未予約境界から間隔が変わる。Device Restartは停止状態へ戻し、BPM / Patternは維持
- 無音ステップもSample Offset付きMarkerで位置を表示。Scheduler遅延は追いつき連打せずSkipし、missedStepsへ記録。Queue overflow時はTransport停止
- Host tests 5本とUndefinedBehaviorSanitizerで検証。GitHub Actions Run `37088456593`でHost tests、Flutter analyze、ARM64 APK Build、Native Library Packaging検査すべて成功。Artifact `11261676177`（2026-10-17まで）。PR #2をmainへ統合済み。P4の実機発音はユーザー確認済み。音に対するステップ表示の遅れが報告された（2026-10-03）
- ステップ表示の50 ms pollingを描画フレーム同期Tickerへ変更し、強調枠のアニメーションを無効化。再生時のみTickerを動かし、変化時だけ再描画。負荷診断は200 ms周期へ分離。Run `37117846090`でC++ tests 5本、Flutter analyze、ARM64 APK build、Native Library Packagingが成功。Artifact `11271858961`、PR #3をmainへ統合済み。ユーザーが修正版について「概ねいい感じに動いている」と確認済み（2026-10-03）。画像ではmissedSteps 0、Queue Overflow 0、Restart 0、48 kHz / 96 frames、p95 0.6% / p99 0.8%。同期の定量測定は未実施
- メインをTransport / BPM / 4×4 Gridへ整理し、診断と単発テストを別画面へ移動。縦画面で全Step表示、狭い横画面ではGridのみScroll。App離脱時はTransport停止、復帰時は自動再生しない
- Widget testsで画面サイズ / 文字倍率 / 操作Intent / 診断更新を検証する。Coreの30分相当×2 Sample Rates、可変Buffer、1000回×2のStop / Play、Device Resetを自動検証する。実Deviceの長時間負荷 / Route Changeは別途必要
- JUCEはP3まで。P4 UI / Control Thread移植と候補比較は後続。技術選定はまだ確定しない

次に進める主題:

1. 整理後のUIとアプリ離脱時の停止を実機 / M4 MacのARM64 Emulatorで確認
2. JUCEへ同等のP4経路を接続し実機比較
3. Device Restart / Route Change、Live Pad独立経路の検証

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

UI整理と安定性自動検証を進めた。次の人による確認は新UI、画面離脱／復帰、実音声Route Change。Technology確定にはJUCE同等経路の実機比較が残る。基本音源 / 複数Track / 保存機能はその後。
