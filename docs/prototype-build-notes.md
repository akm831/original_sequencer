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
- Android runner / APKへのnative library packagingは未実装

Patch version は実際に Reference build machine へ導入した SDK の `flutter --version` 出力を実機 build 結果と一緒に追記します。

### JUCE candidate

- JUCE: 9.0.2
- CMake `FetchContent` の `GIT_TAG` を `9.0.2` に固定

### Android audio candidate

- Oboe: 1.10.0 を P1 の直接 backend 候補とする
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

今回のDart FFI追加後は、このSession環境にFlutter SDK / Android SDKが無いためAndroid実Buildは未確認です。既存Host C ABI contractを維持したまま、Androidでは同じbridge targetをshared library化する準備だけを行っています。

## 現時点で未確認のもの

- Flutter Android app の実機 build / launch
- Flutter Android APKへの `liboriginal_sequencer_flutter_bridge.so` packaging
- Dart FFI の実機 library load
- JUCE Android app の実機 build / launch
- Android NDK version の固定
- Android compileSdk / minSdk / targetSdk の固定
- Oboe callback bring-up
- JUCE audio callback bring-up
- Actual sample rate / callback frames
- Sine output
- Device restart

これらを確認するまでは P0 / P1 完了とはしません。
