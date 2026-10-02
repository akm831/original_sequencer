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
