# Project Status

このファイルは、新しいChatGPT / Codex Sessionで現在地を短時間で復元するための「しおり」です。

詳細仕様をここへ複製しません。確定仕様の正本は各`docs/*.md`、判断理由は`docs/decisions.md`です。

## Phase

Specification / Architecture → Technology Prototype

製品本実装（Phase 1）はまだ開始していません。

Framework / Language / UI Toolkit / Audio Backendは最終確定しておらず、Flutter UI + Native / C++ AudioとJUCE / C++を共通Prototypeで比較してから決定します。

## Product Direction

Touch-firstのGroovebox / Sequencerを設計中です。初期目標は約8 Tracksで、各TrackがSamplerまたはSynth Engineを選択します。Androidを第一ターゲット、Webを第二ターゲットとし、Core Musical LogicはPlatform固有APIへ依存させません。

## Recently Completed

- 主要Domain仕様、960 PPQN Timing、Sample-accurate Scheduling、Audio Thread責務を定義
- Flutter + Native AudioとJUCE / C++を主要Prototype候補として選定
- Framework非依存C++20 Common Reference Audio Coreとhost smoke tests
- Flutter Dart FFI / Android native bridge / Gradle / CMake / Oboe 1.10.0 wiring
- Flutter Android Reference値: compileSdk 36 / minSdk 24 / targetSdk 36 / NDK 28.2.13676358 / CMake 3.22.1
- Flutter Android Debug APK build / native library packaging / Dart FFI runtime loadを確認
- Flutter P1実機成功: `Audio stream: running`、48000 Hz、96 frames/callback、Rendered Frames継続増加、Restart Count 0、220 Hz / amplitude 0.08 test tone実発音
- JUCE 9.0.2 Android Gradle build / Reference Device P1実機成功
- JUCE P1実機値: 48000 Hz、96 frames/callback、Restart Count 0、220 Hz test tone実発音
- Flutter / Oboe backendへ`AudioStreamErrorCallback`による`ErrorDisconnected`検出とstream再openの最小Device Restart経路を追加
- Flutter restart時はCommon Audio Coreを再initializeし、callback frame origin / sine phase / Rendered Framesを新streamとしてリセットする構成にした
- stream stateはmutexで保護し、通常stop時はlockを保持したままOboe streamをcloseしない構成にした

## Current Topic

Technology Prototype: Flutter Device Restart実機検証 → P2 Realtime Diagnostics

現在の到達点:

- JUCE / Flutter双方でAndroid実機P0 / P1基本Bring-up確認済み
- 同一Reference Device上で両Candidateとも48000 Hz / 96 frames/callback
- Flutter Device Disconnect検出 / reopen実装はRepositoryへ追加済みだが、Reference build machineでのcompileと実機route changeは未確認
- JUCE Device Restart挙動は未確認
- P2 Callback Load / Audio Frame Timelineは未確認

次に進める主題:

1. Reference build machineで最新mainをpullしFlutter preflight / debug APK buildを行う
2. `flutter run`で通常P1動作が壊れていないことを確認する
3. Headphone接続/切断等のroute changeでOboe streamが復帰することを確認する
4. 復帰後にtest tone、Actual Sample Rate / Callback Frames、Restart Count増加、Rendered Framesの新timeline開始を確認する
5. Flutter Device Restart結果を記録後、JUCE側で同等試験を行う
6. P2 Callback Duration / Callback Load / Peak / Audio Frame Timeline計測へ進む

PrototypeのScopeと合格条件は`docs/technology-prototype.md`、実装構造とCheckpointは`docs/prototype-implementation-plan.md`を正本とします。候補比較は`docs/framework-comparison.md`、Hard Gateは`docs/platform-audio-requirements.md`です。

## Important Current Decisions

- SequencerとAudio Engineを分離する
- Audio ThreadはUI / Project Model / File I/Oへ直接依存しない
- SchedulerとAudio Callbackを分離し、LookaheadでTimestamp付きAudio Commandを準備する
- Buffer内Eventは可能な限りSample Offset位置で実行する
- Audio Logicを特定Buffer Sizeへ固定しない
- 128 Frames程度をPreferred Target、256 Frames程度をStable Fallbackとする
- PrototypeではFramework非依存C++ Reference Audio Coreを候補間で共有する
- Candidate固有UI / Platform Audio Backendは共有Coreから分離する
- Androidを第一ターゲット、Webを第二ターゲットとする
- Flutter P1 Android backendはOboe 1.10.0をPrefab経由で使用する
- Device Restartでは旧Audio Frame Timelineを継続せず、新streamでframe originを再確立する
- Audio callback自身からstream close/reopenを行わない
- JUCE CandidateのAndroid buildはProjucer Android Studio exporterを使用する
- 最終Technology Decision前にiOSでもMust要件のSmoke Testを行う

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

## Learning Notes

- `docs/learning/audio-midi-basics.md`

## Session Handoff Policy

Session終了前にRepositoryを更新し、新しいSessionではGitHubをSource of Truthとして復元します。

推奨する短い再開指示:

```text
original_sequencerの続きを進めてください。AGENTS.mdとdocs/status.mdを確認し、GitHubの仕様を正本として現在地から再開してください。
```

## Next

Reference build machineで以下を実行する。

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

通常起動で220 Hz test toneと従来の48000 Hz / 96 frames前後のDiagnosticsを確認後、利用可能なら有線/USB headphone等の接続・切断でroute changeを発生させる。復帰後に音が再開し、`audioRestartCount`が1以上へ増え、`renderedFrames`が新しいtimelineとして再び増加することを確認する。Bluetoothは低遅延性能基準には使わない。
