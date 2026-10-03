# Android Groovebox Slice

Flutter Android Version 0.0.3+3。既存アプリに更新Installできる（com.originalsequencer.prototype）。4 Tracks / 4 Patterns / 16 Steps。音は同梱Sampleを使わずC++で合成。

## 操作

- 再生／停止、60〜240 BPM。PlayはStep 1から。
- キック／スネア／ハット／ベースを選びStepをTapしてON / OFF。Long PressでAccent（•）。Accentは通常Velocity 0.65に対して1.0。
- 音量・ミュートで選択TrackのVolume / Mute。Bankごとに独立。
- A〜Dで編集Bankを選択し、再生中は次の未予約小節で切り替え。…は予約、▶は実行済みBank。予約済み約50 msは変更しない。
- 編集後自動保存。AppBar保存ボタンで即保存。App離脱時停止・保存、復帰時自動再生しない。
- 音声診断は別画面。停止中のみ従来の220 Hzテスト利用可。

保存はアプリ内部領域。更新Installで維持、アンインストール／アプリデータ削除では消える。未知Versionや壊れた保存を読んだ場合は上書きを停止する。現在は単一Project、外部Export / Import未実装。

## 人による確認

4音源の聞き分けと音量、Mute、Accent、小節切り替えの聴感、Stepの表示同期、App終了・再起動で全Pattern / BPMが戻ること。M4 MacのARM64 AVDでは操作を確認できるが、Latencyや負荷はAndroid実機で評価する。

## 自動検証

既存6 C++ testsにGroove testを追加。4音源同時合成、Stereo、発音Offset、Mute、小節先頭切り替え、Stop時予約取消、有限Tail、入力検証を確認。FlutterではJSON完全Round Trip、破損／未知Schema拒否、短い画面でのOverflow、上書き停止を確認。CIはanalyze / tests / ARM64 Debug APK / Native Library Packagingまで実行。

これは製品の全Synth / Sampler仕様ではない。現在Bassは固定110 Hz、全Track長は16 Steps、各Track単一Voice。次段階でEngine選択、Notes、Sample Import、Track Lengthを追加する。

## Build済みAPK

[Actions Run 37130262542](https://github.com/akm831/original_sequencer/actions/runs/37130262542)のArtifacts欄から`sequencer-arm64-debug-0a274ad315322d427224fcf1281337c3bfed36d5`をDownloadし、ZIP内のapp-debug.apkを更新Install。Artifact ID 11276104349、2026-10-17まで。C++ 7 tests / Flutter 11 tests / analyze / APK / Native Library Packaging成功。PR #6をmainへ統合済み。
