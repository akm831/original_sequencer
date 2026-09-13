# JUCE Candidate

JUCE/C++ 中心構成の比較候補です。

## P0 current state

- UI source skeleton: `app/Main.cpp`
- Desktop / host build skeleton: `CMakeLists.txt`
- JUCE version: 9.0.2
- Common Reference Audio CoreはCandidate Aと共有する方針

## Android build path

JUCEのCMake APIはAndroid targetをサポートしていないため、Android P0は`juce_add_gui_app()`をAndroid toolchainへ直接渡す構成にはしません。

AndroidではJUCE 9.0.2のProjucer Android Studio exporterを使い、生成されたAndroid Studio / Gradle projectからbuildする方針です。

```text
JUCE 9.0.2 source
  ↓
Projucer
  ↓ Android Studio exporter
Generated Android / Gradle project
  ↓
JUCE app source + Common Reference Audio Core
  ↓
APK
```

Repository rootの`prototypes/juce/CMakeLists.txt`はhost / desktop側のsource smoke build用として残します。誤ってAndroid toolchainで実行した場合は、未サポート経路であることを明示するエラーで停止します。

## Next Android work

1. JUCE 9.0.2のProjucerで最小Android exporter projectを生成可能な`.jucer`設定を追加する
2. Candidate Aと同じAndroid SDK / NDK条件へ合わせる
3. `app/Main.cpp`をAndroid上でlaunchする
4. Common Reference Audio CoreをAndroid exporter側へ接続する
5. P1でJUCE Audio Device callbackを起動し、Actual Sample Rate / Callback FramesをDiagnosticsへ接続する

生成物を無条件にRepositoryへ大量commitするのではなく、再生成手順と固定versionを先に正本化します。
