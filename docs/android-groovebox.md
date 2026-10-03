# Android Groovebox Slice

Flutter Android Version 0.0.5+5。Application IDはcom.originalsequencer.prototype。固定テスト署名の導入前とは署名が合わない可能性があるので、[更新・Backup手順](android-update-backup.md)を先に参照。4 Tracks / 4 Patterns / 16 Steps。音は同梱Sampleを使わずC++で合成。

## 操作

- 再生／停止、60〜240 BPM。Playは編集中のBankのStep 1から。
- キック／スネア／ハット／ベースを選びStepをTapしてON / OFF。Long PressでStep Editorを開きAccent（•）を設定。Accentは通常Velocity 0.65に対して1.0。
- 音色・音量で選択Trackの音色 / Volume / Mute。Bankごとに独立。
- A〜Dで編集Bankを選択し、再生中は次の未予約小節で切り替え。…は予約、▶は実行済みBank。予約済み約50 msは変更しない。
- 編集後自動保存。AppBar保存ボタンで即保存。App離脱時停止・保存、復帰時自動再生しない。
- 音声診断は別画面。停止中のみ従来の220 Hzテスト利用可。

保存はアプリ内部領域。更新Installで維持、アンインストール／アプリデータ削除では消える。未知Versionや壊れた保存を読んだ場合は上書きを停止する。内部は単一Project、右上メニューでJSON Export / Import可能。

## 人による確認

4音源の聞き分けと音量、Mute、Accent、小節切り替えの聴感、Stepの表示同期、App終了・再起動で全Pattern / BPMが戻ること。M4 MacのARM64 AVDでは操作を確認できるが、Latencyや負荷はAndroid実機で評価する。

## 自動検証

既存6 C++ testsにGroove testを追加。4音源同時合成、Stereo、発音Offset、Mute、小節先頭切り替え、Stop時予約取消、有限Tail、入力検証を確認。FlutterではJSON完全Round Trip、破損／未知Schema拒否、短い画面でのOverflow、上書き停止を確認。CIはanalyze / tests / ARM64 Debug APK / Native Library Packagingまで実行。

これは製品の全Synth / Sampler仕様ではない。BassはMIDI Note 24〜84をStep Editorで指定し、Outgoing Slideは直後の有効Stepへつながる。全Track長は16 Steps、各Track単一Voice。

## 808系Percussion / 303系Bass

Kick / Snare / HatはPitch / Tone / Decayを調整。Hat StepのOpenスイッチで長い音へ変更し、Closed HatでChoke。BassはSaw / Square、Cutoff / Resonance / Envelope / Decay。Step長押しでNote / Accent / Slideを設定。音色は新しい未予約Eventから反映。回路の厳密な再現は未実装。

旧Schema 1の編集を保持してSchema 2へ移行。Bassの旧Patternが無音だった場合はそのまま無音を維持するため、Bassを選んでStepを有効にする。音程の初期値はC2。新規ProjectにはデモBass Patternを用意。Note / Slide / Open Hat / 全音色Stateを保存する。

## 前版Build済みAPK

[Actions Run 37130262542](https://github.com/akm831/original_sequencer/actions/runs/37130262542)のArtifacts欄から`sequencer-arm64-debug-0a274ad315322d427224fcf1281337c3bfed36d5`をDownloadし、ZIP内のapp-debug.apkを更新Install。Artifact ID 11276104349、2026-10-17まで。C++ 7 tests / Flutter 11 tests / analyze / APK / Native Library Packaging成功。PR #6をmainへ統合済み。

## Androidの割り込みと停止

PlayはAudio Focus取得後に開始。通話や他アプリへFocusを譲ると停止し、戻っても自動再生しない。新Analog VoiceはStop時に最大20 msでFade。再生中のTrack編集は一括Native Commit。懸念点と未確認項目は[android-audio-review.md](android-audio-review.md)を参照。

## Projectファイル

右上メニューからJSON Export / Import。ImportはReviewと旧内部保存Backup後に反映。読めない旧Schemaも元Raw JSONをExport可能。旧APKからの署名変更時は[android-update-backup.md](android-update-backup.md)を参照して、保存内容を退避してから移行する。
