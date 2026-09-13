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
- 2026-08-12 に Flutter 3.47 stable が公開済み
- Prototype repository では `pubspec.yaml` と UI source skeleton から開始
- Dart側にAndroid用 `dart:ffi` wrapperを追加済み
- Android build時は `original_sequencer_flutter_bridge` をshared libraryとして出力する構成に変更済み
- Flutter Android runnerのGradle / Manifest / Kotlin Activityを追加済み
- `prototypes/flutter_native/platform/android/CMakeLists.txt` をAndroid native buildの入口として追加し、Common Audio CoreとFlutter Native bridgeを同一CMake buildへ接続済み
- `android/app/build.gradle.kts` から上記CMake入口を参照し、APKへ `liboriginal_sequencer_flutter_bridge.so` を組み込める構成にした
- Prototype比較の再現性のためAndroid SDK / NDK値を明示固定した

Reference Android values:

- compileSdk: 36
- minSdk: 24
- targetSdk: 36
- NDK: 28.2.13676358
- externalNativeBuild CMake: 3.22.1

これらはFlutter 3.47系の現行Android defaultと整合する値を、Prototype比較用に動かない値として明示したものです。製品版の最低対応OSを決める判断ではありません。

Flutter patch version は実際に Reference build machine へ導入した SDK の `flutter --version` 出力を実機 build 結果と一緒に追記します。

### JUCE candidate

- JUCE: 9.0.2
- Host / desktop CMake `FetchContent` の `GIT_TAG` を `9.0.2` に固定
- JUCE CMake APIはAndroid targetをサポートしていないため、Android P0/P1はCMake直接生成ではなくProjucerのAndroid Studio exporterを使う
- `prototypes/juce/CMakeLists.txt` はhost / desktop用として維持し、Android toolchainで誤使用した場合は明示的に停止する
- Android再生成元として `prototypes/juce/SequencerPrototype.jucer` を追加済み
- `.jucer`には `app/Main.cpp` とCommon Reference Audio Coreの `AudioCore.cpp` / `AudioCore.h` を登録済み
- Android exporterでは `androidMinimumSDK=24` / `androidTargetSDK=36` を固定済み
- P1 callback bring-up用に `juce_audio_basics` / `juce_audio_devices` / `juce_audio_utils` modulesを追加済み
- `app/Main.cpp` は `juce::AudioAppComponent` でoutput-only Audio Deviceを起動し、callback内でsilenceを維持しながらCommon `AudioCore::render()` を呼ぶ構成へ更新済み
- Actual Sample Rate / Callback Frames / Restart CountはCandidate側のatomic snapshotから5 HzでUI表示する。Audio callbackからUI objectへ直接アクセスしない
- Common `AudioCore::diagnostics()` のcross-thread snapshot契約はP2で整備するため、P1 UIからは直接読まない
- JUCE 9系ではSDK / NDKのローカルpathをExporterへ埋め込む方式ではないため、NDK 28.2.13676358はReference build machine側へ導入し、生成後のGradle projectで利用versionを確認する
- `prototypes/juce/verify_android_export.py` を追加し、`.jucer`の必須設定とProjucer生成後のAndroid projectを機械的に検査できるようにした

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

P0 の host 側では次の smoke test を持ちます。

- `audio_core_smoke`: 共通 `AudioCore` の silence render、Diagnostics、再初期化を確認
- `flutter_bridge_smoke`: Flutter Native 層の C ABI が Core の初期化と Diagnostics を正しく中継することを確認

2026-09-13 時点で、Repository と同じ CMake 構成を再現した host build で両 test が pass することを確認しました。

今回のJUCE P1 source wiring後は、このSession環境にJUCE source / Android SDK / NDK / Projucerが無いため、JUCE host buildとAndroid実Buildは未確認です。Android実機確認まではP1完了扱いにしません。

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

JUCE 9.0.2のAndroid側は、JUCE CMake APIではなくProjucer Android Studio exporterを使います。

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
- `app/Main.cpp` がcompile対象
- `../common/audio_core/src/AudioCore.cpp` がcompile対象
- `../common/audio_core/include` がheader search pathへ入る
- `juce_audio_basics` / `juce_audio_devices` / `juce_audio_utils` が生成projectへ入る
- 実機launch後、Audio callbackが継続して動きActual Sample Rate / Callback Framesが画面へ表示される

実生成物はProjucer / Android Studio versionによって差分が大きくなりやすいため、まず `.jucer` と再生成手順を正本として管理します。

### JUCE Android export preflight

Projucerで生成する前でも、`.jucer`の重要設定は次で検査できます。

```bash
python3 prototypes/juce/verify_android_export.py
```

この段階では `Builds/Android` が無くても警告だけで成功します。確認する項目は以下です。

- Android Studio exporterが存在する
- min SDK 24 / target SDK 36
- `Builds/Android` を生成先にしている
- C++20 compiler flag
- Common Audio Coreのinclude path
- 必須JUCE audio modules
- `Main.cpp` と `AudioCore.cpp` のcompile登録
- Reference build machine上のNDK 28.2.13676358の導入状況（`ANDROID_SDK_ROOT` / `ANDROID_HOME` が設定されている場合）

ProjucerでSave Projectした後は、生成物必須モードで再確認します。

```bash
python3 prototypes/juce/verify_android_export.py --require-generated
```

生成後は、Gradle等のtext fileを走査してmin / target / compile SDKと主要source参照を確認します。NDK versionが生成projectへ明示固定されていない場合は警告を出し、Reference build machineで実際に選択されたNDKを別途確認します。

この検証はAndroid buildそのものの代わりではありません。P0 / P1完了には、引き続き同一Reference Device上でBuild / Launchし、Audio callbackと実値Diagnosticsを確認する必要があります。

## 現時点で未確認のもの

- Flutter Android app の実機 build / launch
- Flutter Android APKへの `liboriginal_sequencer_flutter_bridge.so` packaging実確認
- Dart FFI の実機 library load
- Flutter Android Gradle wrapperのReference build machineでの生成 / 固定
- JUCE Android exporter projectの実生成
- JUCE Android app の実機 build / launch
- Projucer生成側でcompile / target SDK 36とNDK 28.2.13676358が実際に使用されることの確認
- JUCE Audio Device callbackの実機継続動作
- JUCE Actual Sample Rate / Callback Framesの実機値
- Oboe callback bring-up
- Flutter CandidateのActual sample rate / callback frames
- Sine output
- Common Coreのthread-safe Diagnostics snapshot
- Callback Load計測
- Device restart

これらを確認するまでは P0 / P1 完了とはしません。
