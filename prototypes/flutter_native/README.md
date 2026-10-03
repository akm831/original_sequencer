# Flutter + Native Audio Candidate

UIはFlutter、Realtime AudioはCommon C++ Core / Android Oboeへ分離する候補です。

- `app`: Flutter UIとDart FFI
- `native`: C ABI / Android Audio Adapter
- `platform`: Android native build entry

## 現在のCheckpoint

Android P1音出しとP2負荷計測の実機記録は`docs/status.md`を参照してください。
P3のScheduled Trigger経路は実装済み。Hostテスト、GitHub ActionsでのFlutter analyze / ARM64 Android Debug APK buildが成功。実機P3は未確認です。
自動BuildとAPK入手方法は`docs/automated-builds.md`を参照してください。

画面の「Trigger 50 ms test burst」でNative側へtimestamp付きCommandを投入します。
通常は無音、受理されたCommandの指定Sample Offsetから220 Hzの短い検証音を鳴らします。
以前の連続220 Hz Test ToneはP3の音を識別できるよう停止しました。P2の過去の測定値は履歴であり、新しい処理負荷の測定結果ではありません。

- Queue Depth / High Water Mark / Overflow Count
- Trigger Count / Last Trigger Offset
- Callback Timeline / Load / Percentile / Restart Count

を表示します。入力拒否は画面へ表示し、自動Retryはしません。
検証音はMonophonic Retrigger方式です。Poly VoiceおよびLive Padは別Checkpointで実装します。

C ABIのDiagnosticsを拡張したため、DartとNative Libraryは同じRevisionで再Buildしてください。
Hot ReloadだけではNativeの変更は反映されません。

## Host tests

Repository rootで:

```sh
cmake -S prototypes -B build -DORIGINAL_SEQUENCER_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

`audio_trigger_smoke`は発音位置、Stereo、Buffer境界、異なるBuffer分割での波形一致、Burst終了、Late Trigger、Resetを検証します。
`flutter_bridge_smoke`はC ABIから投入してHeadless Renderした実際のSampleとDiagnostics、Overflow、停止・再初期化を検証します。
Headless Render APIはHost専用であり、AndroidにはExportしません。

## P4 16-step loop

「再生」で1小節の16ステップを繰り返します。初期は120 BPM、ステップ1 / 5 / 9 / 13が有効です。ステップをタップしてオン／オフ、Sliderで60〜240 BPMを変更します。オレンジの枠がNative Audioの再生位置です。「停止」は予約Eventを取り消し、現在の短い音は50 ms以内に終了します。再開はステップ1からです。

Native Control Threadが5 ms間隔で約50 ms先まで予約します。FlutterのTickerは描画フレームごとにNative再生位置を確認し、変化時に表示を更新します。ステップ強調枠はアニメーションせず切り替えます。負荷診断は200 ms周期です。UIは音を予約しません。編集は未予約のEventから反映し、BPM変更は次の未予約境界から新しい間隔になります。Device RestartはTransportを停止し、BPMとPatternを維持します。Patternはアプリ終了で失われます。診断のmissedStepsはScheduler遅延でSkipしたStep数です。P3単発テストは停止中のみ利用できます。

## 操作画面と確認環境

メイン画面に再生／停止、BPM、16 Stepsを配置。右上の診断アイコンから、音のテストと更新中の音声診断を開きます。診断画面へ移動しても再生は継続します。別アプリへ移動したり画面が非ActiveになるとTransportを停止し、戻っても自動再生しません。これはPrototypeの動作で、製品版Background Playbackは未決定です。

M4 MacのAndroid StudioではDevice ManagerでARM64 (`arm64-v8a`)の仮想端末を作り、起動したEmulatorへ展開済み`app-debug.apk`をDrag & DropしてInstallできます。Flutter開発環境からは`prototypes/flutter_native/app`をProjectとして開き、起動したAVDを選んでRunします。SDK / NDK等の固定VersionはこのREADMEのBuild手順を参照してください。Mac Native版のAudio Bridgeは未対応なので、Mac上ではAndroid Emulatorを選びます。

自動検証はCore testsに加えFlutter widget testsを実行。一般的な縦画面、Tablet、文字倍率1.8、狭い横画面での操作領域、StepのIntent、診断の更新を確認します。`prototype_sequencer_soak`は44.1 / 48 kHz各30分相当の可変Buffer再生を加速実行し、Timing、有限な出力振幅、Overflow / Skip、Stop / Play 1000回ずつとDevice Resetを確認します。これはCPU DeadlineやDeviceの30分実時間評価ではありません。
