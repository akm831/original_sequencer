# Project Status

このファイルは、新しいChatGPT / Codex Sessionで現在地を短時間で復元するための「しおり」です。

詳細仕様をここへ複製しません。確定仕様の正本は各`docs/*.md`、判断理由は`docs/decisions.md`です。

## Phase

Specification / Architecture → Technology Prototype準備

製品本実装（Phase 1）はまだ開始していません。

Framework / Language / UI Toolkit / Audio Backendは最終確定しておらず、Flutter UI + Native / C++ AudioとJUCE / C++を共通Prototypeで比較してから決定します。

## Product Direction

Touch-firstのGroovebox / Sequencerを設計中です。

初期目標は約8 Tracksで、各TrackがSamplerまたはSynth Engineを選択します。Main SequencerはTR-styleの分かりやすい16-step Surfaceを維持しつつ、将来的にParameter Lock、Probability、Micro Timing、Independent Track Rate等へ拡張します。

Platform展開はAndroidを第一ターゲット、Webを第二ターゲットとし、Core Musical LogicはPlatform固有APIへ依存させません。

## Recently Completed

主要な仕様設計とTechnology Prototype準備は以下まで完了しています。

- Sequencer / Pattern / Persistence / Sampler / Synth / Harmonyの基本仕様
- 960 PPQN Timing、Sample-accurate Scheduling、Lookahead / Live Input分離
- Audio Thread責務、Bounded Queue、Device Restart、Performance Budget方針
- Flutter + Native AudioとJUCE / C++を主要Prototype候補として選定
- Framework非依存C++20 Common Reference Audio Core
- Host `audio_core_smoke` / `flutter_bridge_smoke`
- Flutter Dart FFI / Android native bridge / Gradle / CMake wiring
- Flutter Android Reference値をcompileSdk 36 / minSdk 24 / targetSdk 36 / NDK 28.2.13676358へ固定
- JUCE 9.0.2 `.jucer` Android Studio exporter
- JUCE `AudioAppComponent` output-only callbackとCommon Core render wiring
- JUCE Actual Sample Rate / Callback Frames / Restart Countのatomic diagnostics wiring
- Common `AudioCore::diagnostics()` のcross-thread atomic snapshot
- Common diagnostics同時read/writeのhost ThreadSanitizer確認
- JUCE 9.0.2 ProjucerからAndroid projectを実生成
- JUCE Android C++20をraw `-std=c++20`ではなくProjucer `cppLanguageStandard=20` → generated `CMAKE_CXX_STANDARD=20`で指定
- JUCE Android生成projectでSDK / NDK参照を修正し、NDK 28.2.13676358で`./gradlew assembleDebug`成功
- Native build cacheとProjucer生成物を`.gitignore`へ追加
- `verify_android_export.py`を現行Projucer設定へ追従し、`.jucer`の`cppLanguageStandard=20`と生成projectの`CMAKE_CXX_STANDARD=20`を検査するよう更新
- `verify_android_export.py --require-generated`でmin / target / compile SDK、C++ standard、source wiring、NDK pinを機械検査できるよう強化

## Current Topic

Technology Prototype P0 / P1実機Bring-up + P2準備

現在の到達点:

- Common Audio Core、Flutter bridge、JUCE callback source wiringは実装済み
- JUCE Android生成projectのGradle buildは成功済み
- JUCE Android export preflightは現行のC++ Language Standard設定へ追従済み
- 生成projectのSDK / C++20 / NDK / source wiringを`--require-generated`で再検査できる
- JUCE Androidの実機Install / Launch、Audio Device callback継続動作、Actual Sample Rate / Callback Frames、実発音は未確認
- Flutter Androidの実機Build / Launch、APK内Native library packaging、Dart FFI実ロードも未確認
- Callback Load計測などP2の残りは未実装

次に進める主題:

- Reference build machineで`python3 prototypes/juce/verify_android_export.py --require-generated`を実行してローカル生成projectを再検査する
- JUCE Android APKをReference DeviceへInstall / Launchする
- JUCE Audio Device callbackの実機継続動作とActual Sample Rate / Callback Frames表示を確認する
- Flutter側も同一Android Reference DeviceでBuild / Launchし、APKへのNative library packagingとDart FFI実ロードを確認する
- 両候補でsilenceからsine outputへ進める
- P2でCallback Load計測 / Audio Frame Timelineへ進む
- P2 Common diagnostics snapshotをFlutter / JUCE双方の低頻度UI表示へ統合する

PrototypeのScopeと合格条件は`docs/technology-prototype.md`、実装構造とCheckpointは`docs/prototype-implementation-plan.md`を正本とします。

候補比較の基準は`docs/framework-comparison.md`、Hard Gateは`docs/platform-audio-requirements.md`とします。

Build手順とversion固定状況は`docs/prototype-build-notes.md`を参照します。

## Important Current Decisions

- SequencerとAudio Engineを分離する
- TrackがSampler / Synth等のEngineを選択する
- v0.1 Main Gridは1 Step = 1/16 note
- Timing内部目標は960 PPQN
- Audio ThreadはUI / Project Model / File I/Oへ直接依存しない
- SchedulerとAudio Callbackを分離し、LookaheadでTimestamp付きAudio Commandを準備する
- Buffer内Eventは可能な限りSample Offset位置で実行する
- Audio Logicを特定Buffer Sizeへ固定しない
- 128 Frames程度をPreferred Target、256 Frames程度をStable Fallbackとする
- Sequencer Lookaheadは約50 msを初期候補とするが、Live InputはそのLookaheadを待たない
- Framework選定ではRealtime AudioのMust要件を先に評価する
- Prototype主要候補はFlutter UI + Native / C++ Audio LayerとJUCE / C++中心構成
- PrototypeではFramework非依存の小さなC++ Reference Audio Coreを候補間で共有する
- Candidate固有のUI / Platform Audio Backendは共有Coreから分離する
- Androidを第一ターゲット、Webを第二ターゲットとし、CoreをAndroid専用APIへ依存させない
- Flutter PrototypeのAndroid SDK / NDKは比較再現性のため明示固定する
- JUCE CandidateのAndroid buildはJUCE CMake APIではなくProjucer Android Studio exporterを使用する
- JUCE P1ではUI ThreadとAudio Callbackの間で直接UI objectを共有せず、軽量snapshotを介す
- 最終Technology Decision前にiOSでもMust要件のSmoke Testを行う

## Primary References

現在のTopicを再開するときは、まず以下を確認します。

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

必要になった場合のみ、関連する詳細仕様を追加で読みます。

## Learning Notes

基礎知識は仕様書へ混ぜず、`docs/learning/`へ蓄積します。

現在:

- `docs/learning/audio-midi-basics.md`

## Session Handoff Policy

Chatが長くなった場合、巨大な引き継ぎPromptを作成しません。Session終了前にRepositoryを更新し、新しいSessionではGitHubをSource of Truthとして復元します。

推奨する短い再開指示:

```text
original_sequencerの続きを進めてください。AGENTS.mdとdocs/status.mdを確認し、GitHubの仕様を正本として現在地から再開してください。
```

## Next

Technology PrototypeのP0 Android build確認を完了し、P1の実機Bring-upへ進む。

1. Reference build machineで`python3 prototypes/juce/verify_android_export.py --require-generated`を実行する
2. JUCE Android APKをReference DeviceへInstall / Launchする
3. JUCE実機でAudio callback継続動作とActual Sample Rate / Callback Frames表示を確認する
4. Flutter / JUCEを同一Android Reference DeviceでBuild / Launchする
5. Flutter APK内の`liboriginal_sequencer_flutter_bridge.so`とDart FFI実ロードを確認する
6. 両候補でsilenceからsine outputへ進める
7. P2でCallback Load計測 / Audio Frame Timelineを実装する
8. Common Coreのthread-safe Diagnostics snapshotをFlutter / JUCE双方の低頻度UI表示へ統合する
