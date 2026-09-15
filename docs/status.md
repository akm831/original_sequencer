# Project Status

このファイルは、新しいChatGPT / Codex Sessionで現在地を短時間で復元するための「しおり」です。

詳細仕様をここへ複製しません。確定仕様の正本は各`docs/*.md`、判断理由は`docs/decisions.md`です。

## Phase

Specification / Architecture → Technology Prototype

製品本実装（Phase 1）はまだ開始していません。

Framework / Language / UI Toolkit / Audio Backendは最終確定しておらず、Flutter UI + Native / C++ AudioとJUCE / C++を共通Prototypeで比較してから決定します。

## Product Direction

Touch-firstのGroovebox / Sequencerを設計中です。

初期目標は約8 Tracksで、各TrackがSamplerまたはSynth Engineを選択します。Main SequencerはTR-styleの分かりやすい16-step Surfaceを維持しつつ、将来的にParameter Lock、Probability、Micro Timing、Independent Track Rate等へ拡張します。

Platform展開はAndroidを第一ターゲット、Webを第二ターゲットとし、Core Musical LogicはPlatform固有APIへ依存させません。

## Recently Completed

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
- Flutter P1用Oboe 1.10.0 backend / start-stop FFI / 5 Hz Diagnostics UIを実装
- Flutter Oboe callbackをCommon Audio Coreへ接続し、220 Hz / amplitude 0.08 test toneを出力
- Flutter P1をReference Deviceで実機確認: `Audio stream: running`、Actual Sample Rate 48000 Hz、Callback Frames 96、Rendered Frames継続増加、Audio Restart Count 0、220 Hz test tone実発音
- JUCE 9.0.2 Projucer Android Studio exporterとCommon Core wiring
- JUCE Android C++20 / SDK / NDK / source wiring preflight
- JUCE Android Gradle build成功
- JUCE Android APKをReference DeviceへInstall / Launchし、220 Hz test toneの実発音を確認
- JUCE実機DiagnosticsでActual Sample Rate 48000 Hz / Callback Frames 96 / Restart Count 0を確認

## Current Topic

Technology Prototype: Device Lifecycle / P2 Realtime Diagnostics

現在の到達点:

- Common Audio Core、Flutter bridge、JUCE callback source wiringは実装済み
- JUCE CandidateはAndroid実機でP0 / P1基本Bring-up確認済み
- Flutter CandidateもAndroid実機でP0 / P1基本Bring-up確認済み
- 同一Reference Device上で両CandidateともActual Sample Rate 48000 Hz / Callback Frames 96 / Restart Count 0
- 両Candidateとも220 Hz / amplitude 0.08 test toneの実発音を確認済み
- FlutterはRendered Framesが継続増加し、Native callback → Common Audio Coreの連続動作を実機確認済み
- Device Restart / Background-Foreground / Route Change時の安全な復帰は未確認
- P2 Callback Load / Audio Frame Timelineは未確認

次に進める主題:

1. Flutter / Oboe側にDevice Disconnect検出と安全な再openの最小Vertical Sliceを追加する
2. Reference DeviceでHeadphone route change等を使って旧callback停止 → stream再open →新Sample Rate / Callback Frames取得を確認する
3. Restart CountとFrame Timeline resetをDiagnosticsで確認する
4. JUCE側でも同等のDevice Restart試験を行う
5. P2としてCallback Duration / Callback Load / PeakとAudio Frame Timelineの計測へ進む

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

Flutter / OboeのDevice Restart検証を最小Vertical Sliceで追加する。Audio callback内ではstreamのclose/reopenを行わず、Oboe error callback / management側で安全に再openする。再起動後は旧Audio Frame Timelineを継続せず、新しいframe originを0から開始し、Actual Sample Rate / Callback Framesを再取得する。

実装後、Reference build machineでpreflight → debug APK build → packaging check → `flutter run`を行い、Headphone接続/切断等のroute changeでtest toneが復帰すること、`audioRestartCount`が増えること、`renderedFrames`が新timelineとして再開することを確認する。
