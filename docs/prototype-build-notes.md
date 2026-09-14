# Technology Prototype Build Notes

この文書は Technology Prototype の再現性を保つため、実装開始時点で使う Toolchain / Framework version と build 上の前提を記録します。

製品版の最終 Technology Decision ではありません。

## 2026-09-13: P0 開始時点

### 共通 C++ Core

- Language target: C++20
- Build system: CMake
- Minimum CMake version: 3.22
- Core target: static library
- Framework / platform API への依存: なし

理由:

- Flutter Native 層と JUCE 候補の両方から同じ Core を参照しやすい
- Android NDK / desktop host の両方で小さな smoke test を回しやすい
- Prototype で必要な責務境界を保ったまま、製品版 build system を先に固定しすぎない

### Flutter candidate

- Stable family: Flutter 3.47
- Prototype repository では `pubspec.yaml` と UI source skeleton から開始
- Dart側にAndroid用 `dart:ffi` wrapperを追加済み
- Android build時は `original_sequencer_flutter_bridge` をshared libraryとして出力する
- Flutter Android runnerのGradle / Manifest / Kotlin Activityを追加済み
- `prototypes/flutter_native/platform/android/CMakeLists.txt` をAndroid native buildの入口としてCommon Audio CoreとFlutter Native bridgeを同一buildへ接続済み

Reference Android values:

- compileSdk: 36
- minSdk: 24
- targetSdk: 36
- NDK: 28.2.13676358
- externalNativeBuild CMake: 3.22.1

これらはPrototype比較用の固定値であり、製品版の最低対応OSを確定するものではありません。

### JUCE candidate

- JUCE: 9.0.2
- Host / desktop CMake `FetchContent` の `GIT_TAG` を `9.0.2` に固定
- JUCE CMake APIはAndroid targetをサポートしていないため、Android P0/P1はProjucerのAndroid Studio exporterを使う
- `prototypes/juce/CMakeLists.txt` はhost / desktop用として維持し、Android toolchainで誤使用した場合は停止する
- Android再生成元は `prototypes/juce/SequencerPrototype.jucer`
- `.jucer`には `app/Main.cpp` とCommon Reference Audio Coreの `AudioCore.cpp` / `AudioCore.h` を登録
- Android exporterでは `androidMinimumSDK=24` / `androidTargetSDK=36` を固定
- C++20はrawな`-std=c++20` compiler flagではなく、Projucer project rootの `cppLanguageStandard="20"` で指定する
- Projucer生成後のAndroid projectでは `CMAKE_CXX_STANDARD 20` として反映されることを検査する
- P1 callback bring-up用に `juce_audio_basics` / `juce_audio_devices` / `juce_audio_utils` modulesを追加済み
- `app/Main.cpp` は `juce::AudioAppComponent` でoutput-only Audio Deviceを起動し、callback内でCommon `AudioCore::render()` を呼ぶ
- P1の実機Audio Path確認用に、JUCE callback側で220 Hz / amplitude 0.08の低音量sine test toneを出力する
- Actual Sample Rate / Callback Frames / Restart CountはCandidate側atomic snapshotから5 HzでUI表示する
- Audio callbackからUI objectへ直接アクセスしない
- NDK 28.2.13676358はReference build machineへ導入し、生成project側でも利用versionを明示する

Reference JUCE source layout:

```text
workspace/
├─ original_sequencer/
└─ JUCE/                 # tag 9.0.2
```

この配置では `.jucer` から `../../../JUCE/modules` を参照します。JUCE sourceを別場所へ置く場合はProjucerでmodule pathだけ変更します。

### Android audio candidate

- Oboe: 1.10.0 を P1 のFlutter + Native側直接 backend候補とする
- Actual sample rate / callback frames は要求値ではなく stream 開始後の実値を Diagnostics へ記録する

JUCE 内部の Android backend と Oboe を「同じ実装」とは扱いません。比較では各 Candidate の実際の callback path を記録します。

## Host smoke test

Repository root から:

```bash
cmake -S prototypes -B build/prototypes
cmake --build build/prototypes
ctest --test-dir build/prototypes --output-on-failure
```

P0 のhost側では次のsmoke testを持ちます。

- `audio_core_smoke`: 共通 `AudioCore` の silence render、Diagnostics、再初期化を確認
- `flutter_bridge_smoke`: Flutter Native 層の C ABI が Core の初期化と Diagnostics を正しく中継することを確認

Common diagnosticsの同時read/writeはhost ThreadSanitizerでもdata raceなしを確認済みです。

## Flutter Android build wiring

Android runnerの主な経路:

```text
Flutter app
  ↓
android/app/build.gradle.kts
  ↓ externalNativeBuild
prototypes/flutter_native/platform/android/CMakeLists.txt
  ├─ prototypes/common/audio_core
  └─ prototypes/flutter_native/native
       ↓
liboriginal_sequencer_flutter_bridge.so
  ↓
Dart DynamicLibrary.open(...)
```

Reference build machineでは次を確認します。

```bash
cd prototypes/flutter_native/app
flutter pub get
flutter build apk --debug
```

その後、APK内に `liboriginal_sequencer_flutter_bridge.so` が対象ABIごとに含まれることを確認し、実機launch時に画面上の `Native bridge: loaded` を確認します。

## JUCE Android build wiring

JUCE 9.0.2のAndroid側はProjucer Android Studio exporterを使います。

```text
JUCE 9.0.2 / Projucer
  + prototypes/juce/SequencerPrototype.jucer
  ↓ Android Studio exporter
prototypes/juce/Builds/Android
  ↓
JUCE Audio Device callback
  ↓
JUCE app source
  + Common Reference Audio Core
  ↓
APK
```

`.jucer`を開いてSave ProjectするとAndroid Studio projectを再生成できます。Reference build machineでは生成後に以下を確認します。

- min SDK: 24
- target / compile SDK: 36
- NDK: 28.2.13676358
- C++ language standard: C++20
- `app/Main.cpp` がcompile対象
- `../common/audio_core/src/AudioCore.cpp` がcompile対象
- `../common/audio_core/include` がheader search pathへ入る
- `juce_audio_basics` / `juce_audio_devices` / `juce_audio_utils` が生成projectへ入る
- 実機launch後、Audio callbackが継続して動きActual Sample Rate / Callback Framesが画面へ表示される
- 220 Hzの低音量test toneが端末Audio Outputから安定して聞こえる

実生成物はProjucer / Android Studio versionによって差分が大きくなりやすいため、`.jucer` と再生成手順を正本として管理し、`Builds/Android` 自体はGit管理対象にしません。

### JUCE Android export preflight

Projucerで生成する前でも、`.jucer`の重要設定は次で検査できます。

```bash
python3 prototypes/juce/verify_android_export.py
```

この段階では `Builds/Android` が無くても警告だけで成功します。確認する項目は以下です。

- Android Studio exporterが存在する
- min SDK 24 / target SDK 36
- `Builds/Android` を生成先にしている
- Project rootの `cppLanguageStandard="20"`
- Common Audio Coreのinclude path
- 必須JUCE audio modules
- `Main.cpp` と `AudioCore.cpp` のcompile登録
- Reference build machine上のNDK 28.2.13676358の導入状況（`ANDROID_SDK_ROOT` / `ANDROID_HOME` が設定されている場合）

ProjucerでSave Projectした後は、生成物必須モードで再確認します。

```bash
python3 prototypes/juce/verify_android_export.py --require-generated
```

生成後はtext fileを走査し、以下を検査します。

- min SDK 24
- target SDK 36
- compile SDK 36
- `CMAKE_CXX_STANDARD 20`
- `Main.cpp` / `AudioCore.cpp` のsource wiring
- Common Audio Core include path
- NDK 28.2.13676358の明示pin

SDK、C++ standard、source wiring、NDK pinが期待値と一致しない場合、`--require-generated`の検査は失敗します。これにより、ローカル生成物がPrototype比較用のReference設定からずれたままbuildされることを防ぎます。

この検証はAndroid buildそのものの代わりではありません。P0 / P1完了には、引き続き同一Reference Device上でBuild / Launchし、Audio callbackと実値Diagnosticsを確認する必要があります。

## 2026-09-14: JUCE Android build / P1実機確認

- JUCE 9.0.2 Projucerから `prototypes/juce/Builds/Android` を実生成済み
- C++20指定を `CMAKE_CXX_STANDARD=20` へ整理済み
- SDK / NDK参照をReference値へ合わせ、NDK 28.2.13676358で `./gradlew assembleDebug` 成功を確認済み
- 生成物とNative build cacheは再生成可能なため `.gitignore` へ追加済み
- `verify_android_export.py` は旧 `extraCompilerFlags=-std=c++20` 前提を廃止し、ProjucerのC++ Language Standard設定と生成後のCMake設定を検査するよう更新済み
- `verify_android_export.py --require-generated` はReference build machineでPASSを確認済み
- P1実機Bring-upでAudio Device経路を耳でも確認できるよう、JUCE callbackへ220 Hz / amplitude 0.08のsine test toneを追加済み
- test tone追加後の `./gradlew assembleDebug` が成功
- Debug APKをReference DeviceへInstall / Launchし、端末スピーカーから220 Hz test toneの実発音を確認
- 実機Diagnostics: Actual Sample Rate = 48000 Hz
- 実機Diagnostics: Callback Frames = 96
- 実機Diagnostics: Audio Restart Count = 0（確認時点）
- JUCE CandidateのP0とP1基本Audio Bring-upは実機で確認済み

## 現時点で未確認のもの

- Flutter Android app の実機 build / launch
- Flutter Android APKへの `liboriginal_sequencer_flutter_bridge.so` packaging実確認
- Dart FFI の実機 library load
- JUCE Audio Device callbackの長時間安定性
- JUCE Device restart時の復帰挙動とRestart Count増加
- Oboe callback bring-up
- Flutter CandidateのActual sample rate / callback frames
- Flutter Candidateのsine output
- Callback Load計測

JUCE側のP0 / P1基本Bring-upは確認済みですが、Candidate比較完了にはFlutter側の同等確認とDevice Restart / P2計測が残っています。
