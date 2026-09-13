# JUCE Candidate

JUCE/C++ 中心構成の比較候補です。

## P0 current state

- UI source skeleton: `app/Main.cpp`
- Desktop / host build skeleton: `CMakeLists.txt`
- Android Projucer project: `SequencerPrototype.jucer`
- JUCE version: 9.0.2
- Common Reference Audio CoreはCandidate Aと共有する方針

## Android build path

JUCEのCMake APIはAndroid targetをサポートしていないため、Android P0は`juce_add_gui_app()`をAndroid toolchainへ直接渡す構成にはしません。

AndroidではJUCE 9.0.2のProjucer Android Studio exporterを使い、`SequencerPrototype.jucer`から生成されたAndroid Studio / Gradle projectをbuildします。

```text
JUCE 9.0.2 source
  ↓
Projucer + SequencerPrototype.jucer
  ↓ Android Studio exporter
Builds/Android
  ↓
JUCE app source + Common Reference Audio Core
  ↓
APK
```

Repository rootの`prototypes/juce/CMakeLists.txt`はhost / desktop側のsource smoke build用として残します。誤ってAndroid toolchainで実行した場合は、未サポート経路であることを明示するエラーで停止します。

## JUCE source layout

`.jucer`はJUCE 9.0.2 sourceをRepositoryの隣へcheckoutする構成をReferenceとします。

```text
workspace/
├─ original_sequencer/
└─ JUCE/                 # tag 9.0.2
```

この配置では`prototypes/juce/SequencerPrototype.jucer`から`../../../JUCE/modules`でJUCE modulesを参照できます。

例:

```bash
git clone --branch 9.0.2 --depth 1 https://github.com/juce-framework/JUCE.git
```

JUCEを別の場所へ置く場合は、Projucerでmodule pathだけ環境に合わせて変更します。Application sourceやCommon Audio Coreの相対pathは変更しません。

## Android Reference values

Candidate Aと比較条件を揃えるため、次をReference値とします。

- compile / target SDK: 36
- min SDK: 24
- NDK: 28.2.13676358

`SequencerPrototype.jucer`では`androidMinimumSDK=24`と`androidTargetSDK=36`を固定しています。

JUCE 9系ではAndroid SDK / NDKのローカルpathを各Exporterへ埋め込む方式ではなく、Android Studio / SDK Manager側の環境を利用します。そのためNDK 28.2.13676358はReference build machineへインストールし、生成後のGradle projectがそのversionを使っていることを実build時に確認します。

## Projucerでの再生成

1. JUCE 9.0.2のProjucerを起動する
2. `SequencerPrototype.jucer`を開く
3. Android Studio exporterが選択されていることを確認する
4. Save Projectを実行する
5. `Builds/Android`をAndroid Studioで開く
6. Reference build machineでSDK 36 / NDK 28.2.13676358を利用してbuildする

`.jucer`には`app/Main.cpp`に加えてCommon Reference Audio Coreの`AudioCore.cpp` / `AudioCore.h`も登録してあります。これによりCandidate BでもCandidate Aと同じCore sourceを使う境界を維持します。

## Next Android work

1. JUCE 9.0.2 Projucerで`Builds/Android`を実生成する
2. 生成されたGradle設定でcompile / target SDK 36、min SDK 24、NDK 28.2.13676358を実確認する
3. `app/Main.cpp`をAndroid実機上でlaunchする
4. Common Reference Audio CoreがAndroid buildへ入ることを確認する
5. P1でJUCE Audio Device callbackを起動し、Actual Sample Rate / Callback FramesをDiagnosticsへ接続する

生成物を無条件にRepositoryへ大量commitするのではなく、再生成元の`.jucer`と手順を正本として維持します。
