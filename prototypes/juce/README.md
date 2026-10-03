# JUCE Candidate

JUCE/C++ 中心構成の比較候補です。

## Current state

- UI source: `app/Main.cpp`。Play / Stop、60〜240 BPM、16-step Grid、診断切り替え
- JUCE 9.0.2、Framework非依存のCommon C++ Core / SchedulerをFlutterと共有
- Native Control Threadが5 ms周期で予約し、Audio CallbackでPlanar / Sample Offset Render
- UIは60 Hzで再生位置を観測、負荷診断は5 Hz。CallbackからUI Objectを操作しない
- Desktop / host build: `CMakeLists.txt`。Android: `SequencerPrototype.jucer`から自動Export
- Android APK compile / link / packaging成功（Run `37127513073`）。P4実機確認は未実施

## Android build path

JUCEのCMake APIはAndroid targetをサポートしていないため、Android P0/P1は`juce_add_gui_app()`をAndroid toolchainへ直接渡す構成にはしません。

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

Repository rootの`prototypes/juce/CMakeLists.txt`はhost / desktop側のbuild用として残します。誤ってAndroid toolchainで実行した場合は、未サポート経路であることを明示するエラーで停止します。

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

`.jucer`には`app/Main.cpp`、Common Reference Audio Core、P1で必要な`juce_audio_basics` / `juce_audio_devices` / `juce_audio_utils` modulesを登録しています。

## Callbackと比較の責務

AudioAppComponent::getNextAudioBlockはCommon CoreのRenderとTiming記録のみ。UI / Scheduler / LifecycleのMutexを取得しません。共有Schedulerへの操作はControl側で直列化します。Device再準備時は旧Timeline / Commandを捨て、Transport停止状態へ戻します。

`.jucer`と生成後設定Scriptを正本として維持し、生成Gradle Projectは大量Commitしません。次の検証はP4の実機比較です。

## 2026-10-02: P3 Scheduled Trigger / P2 Diagnostics

連続Test Toneを止め、`Trigger 50 ms test burst`ボタンからCommon CoreへScheduled Triggerを投入します。Flutterと同じ50 ms / 220 Hz / 最大8%のMonophonic検証音です。Live Pad / Poly Voiceは後続Checkpointです。

JUCEのPlanar AudioBufferはCommon CoreへChannel Pointerと`startSample`を渡します。Callback内でInterleaved変換用Memoryを確保せず、対象Sample領域だけをRenderします。Host testでAndroid Interleaved出力との一致を確認しています。

Command投入と`prepareToPlay` / `releaseResources`をControl側Mutexで直列化します。通常の`getNextAudioBlock`はMutexを取得しません。Queue / Trigger Diagnosticsに加え、Flutterと同じCoreのCallback Load / P95 / P99 / Peak / Timelineを表示します。

当時はJUCE 9.0.2 Headerによる構文検査のみでした。その後P4のAndroid compile / linkが成功しました。JUCE実機P2〜P4は未確認です。既存のP1実機結果は過去の連続Test Toneに対する結果です。

## P4 Play / Stop、BPM、16 Steps

共有C++ PrototypeSequencerをNative Control Threadから5 ms周期で進め、約50 ms先まで予約します。PlayはStep 1から開始、Stopは旧予約を世代番号で無効化し、現在の短いVoiceは50 ms以内に終了します。BPM / Patternは未予約Eventから反映します。UIは60 HzでNativeの実行済み位置を読み、現在StepをOrangeで表示します。診断は5 Hzです。

Audio diagnosticsから診断画面へ切り替えても再生は続きます。Test soundは停止中のみ。App Suspend時は停止し、Resume時は自動再生しません。Device再準備時もTransportは停止し、BPM / Patternは維持します。

新Application ID: `com.originalsequencer.juceprototype`。Flutterの`com.originalsequencer.prototype`と併存します。旧JUCE P1版は旧IDなので、新版は別AppとしてInstallされます。

## 自動Android Build

`.github/workflows/juce-prototype-ci.yml`がJUCE 9.0.2の固定CommitからProjucerをbuildし、.jucerをExportします。`configure_android_export.py`でNDK 28.2.13676358 / JDK 17とJava 8 bytecodeを設定、`verify_android_export.py --require-generated`で確認してARM64 APKをbuildします。JNI同梱を検査し、`juce-arm64-debug.apk`とbuild-info.jsonを14日間Artifactへ保存します。

Macで手動再生成する場合も、Save Project後に次を実行してください:

```sh
python3 prototypes/juce/configure_android_export.py
python3 prototypes/juce/verify_android_export.py --require-generated
```

M4 MacのAndroid StudioではARM64 AVDへAPKをInstallしてUI / 基本発音を確認できます。実音声Latency、Route Change、継続負荷はAndroid実機で比較します。比較手順は`docs/framework-comparison.md`のP4項を参照してください。

比較用APK: https://github.com/akm831/original_sequencer/actions/runs/37127513073 。Artifacts内の`juce-arm64-debug.apk`をInstallしてください（保存期限2026-10-17）。
