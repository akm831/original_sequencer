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
- Flutter Android用 `verify_android_setup.py` でSDK / NDK / CMake / Common Core / shared library / Dart FFI / APK packagingを機械検査
- Flutter Android Debug APK build成功
- Flutter Android APK内に`liboriginal_sequencer_flutter_bridge.so`がarm64-v8a / armeabi-v7a / x86_64向けに含まれることを確認
- Flutter AndroidをReference DeviceでLaunchし、Dart FFI実ロードと`Native bridge: loaded`を確認
- JUCE 9.0.2 Projucer Android Studio exporterとCommon Core wiring
- JUCE Android C++20 / SDK / NDK / source wiring preflight
- JUCE Android Gradle build成功
- JUCE Android APKをReference DeviceへInstall / Launchし、220 Hz test toneの実発音を確認
- JUCE実機DiagnosticsでActual Sample Rate 48000 Hz / Callback Frames 96 / Restart Count 0を確認
- Flutter P1用にOboe 1.10.0 Prefab dependency / Android audio backend / start-stop FFI / 5 Hz Diagnostics UIを実装
- Flutter Oboe callbackはCommon Audio Coreを通し、JUCEと同じ220 Hz / amplitude 0.08 test toneを出力する構成にした

## Current Topic

Technology Prototype P1比較継続 + P2準備

現在の到達点:

- Common Audio Core、Flutter bridge、JUCE callback source wiringは実装済み
- JUCE CandidateはAndroid実機でP0 / P1基本Bring-up確認済み
- JUCE実機値はActual Sample Rate 48000 Hz / Callback Frames 96 / Restart Count 0
- Flutter CandidateはAndroid runner / Gradle / CMake / Dart FFI wiring済み
- Flutter CandidateはAndroid実機でP0完了: APK build / native library packaging / Dart FFI loadを確認済み
- Flutter P0確認時はAudio Device未接続だったためDiagnosticsが0であることを確認済み
- Flutter P1のOboe 1.10.0 callback / sine output / Diagnostics refresh実装はRepositoryへ追加済み
- Flutter P1実装のReference build machine上での再buildと実機Audio確認は未確認
- JUCE / Flutter双方のDevice Restart挙動と、P2 Callback Load / Audio Frame Timelineは未確認

次に進める主題:

1. Reference build machineで最新mainをpullする
2. 更新後のFlutter preflightを実行する
3. Flutter Android appを再buildし、Oboe Prefab + C++ bridgeがcompile/linkできることを確認する
4. Build後preflightでAPK packagingを再確認する
5. Reference Deviceへ`flutter run`し、`Audio stream: running`を確認する
6. 端末から220 Hz test toneが安定して聞こえることを確認する
7. Flutter DiagnosticsでActual Sample Rate / Callback Frames / Rendered Frames増加 / Restart Countを記録する
8. JUCE / Flutter双方でDevice Restart挙動を検証する
9. P2でCallback Load計測 / Audio Frame Timelineへ進む

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
- Flutter P1 Android backendはOboe 1.10.0をPrefab経由で使用し、Common Coreから分離する
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

Reference build machineで以下を順に実行し、Flutter CandidateのP1実機確認を進める。

```bash
cd /Volumes/DevSDK/Development/Projects/original_sequencer
git pull --ff-only
python3 prototypes/flutter_native/verify_android_setup.py
cd prototypes/flutter_native/app
flutter pub get
flutter build apk --debug
cd ../../..
python3 prototypes/flutter_native/verify_android_setup.py --require-apk
cd prototypes/flutter_native/app
flutter run
```

実機画面で`Native bridge: loaded` / `Audio stream: running`を確認し、220 Hz test toneの実発音、Actual Sample Rate、Callback Frames、Rendered Frames増加、Audio Restart Countを記録する。
