# Android音源Sliceの懸念点と確認記録

CI Run `37135809703`でC++ 9 tests / Flutter 24 tests / APK / Exported FFI Symbols / 固定署名の証明書検査成功。PR #8をmainへ統合済み。

## 修正・自動確認済み

| 懸念 | 対応と検証 |
|---|---|
| 再生中にMaskだけ新しくNotesが旧状態になる | Track Stateを一括検証・Commit。C ABIで不正更新が一部適用されないことを確認 |
| Stop後に長いKick / Open Hat / Slideが残る | 世代変更で最大20 ms Fade。停止後silenceと1000回Stop / Playを検証 |
| RetriggerのResetによる不連続 | 直前Sampleから約2 msで移行。最初のSample連続性を検証 |
| 休符をまたぐSlide | 直後の有効Stepと同Patternのみ。休符ありではSlide ON / OFFの波形一致を検証 |
| 密な4 Track演奏でQueueが溢れる | 44.1 / 48 kHz各5分相当、可変Buffer、全Step有効、最大音色設定、Pattern切り替えを検証 |
| Resonance / Cutoff極値でNaNや大振幅 | 全音源と両波形の極値を検査、Mix出力を±0.8へ制限 |
| 古い保存が新音源へ更新時に消える | Schema 1→2 Migrationで元Mask / Accent / Volume / Mute / BPMを保持 |
| 保存ファイルが巨大だと読み込み後の検査では遅い | Android I/Oは最大256 KiB+1までしか読み込まず上書き停止 |
| 文字拡大でBass NoteがGridから溢れる | Note行に応じて最小Grid高を増やしScroll。360×640 / 文字1.8倍を検査 |
| Stop直前に選んだ編集Bankと次のPlayが違う | Play時に編集Bankを明示し、Step 1から開始 |
| 通話・他アプリに音声使用権を譲れない | Audio Focusを取得し、Lossでは停止、Gainでは自動再生しない。遅いGrantの取消をUnit test |
| CI署名が変わり更新Installできずデータが消える | 公開テスト用署名を固定し証明書をCI検査。初回署名移行はdocs/android-update-backup.mdの手順でBackup |
| 外部Importで旧Projectを不用意に上書きする | Parse → Review → Backup → Commitの順に制限。Cancel / 不正JSON / Backup失敗をUnit test |
| APKに新FFI Entryが同梱されない | ARM64 .soのExported Symbols検査 |

## 人の確認が必要

1. 音色: Kickの低音とDecay、SnareのBody / Noise、Hatの金属感 / Choke、BassのResonance / Accent / Slideをイヤホンとスピーカーで確認。回路の厳密な再現ではなく、特徴合成の第一版。
2. 実負荷: 全4 Tracks / 240 BPM / Resonance最大 / Open Hatで10分以上。missedSteps / Overflow / Restartとp95 / p99を観測。Hostの5分相当testは実時間のAndroid Load testではない。
3. 保存: 署名が一致するAPKの更新Installで旧Pattern維持、Note / Slide / 音色編集後のApp再起動。アンインストールはデータ削除となる。
4. ファイル: 端末Files / クラウド保存先へのExport / Import、キャンセル、不正JSON、旧Schemaを確認。初回の署名変更時は旧版を消す前にバックアップ。
5. Audio Focus: 他の音楽アプリ再生、着信、通知、App切り替え後に勝手に再開しないこと。API 24〜25とAndroid 15以降も対象。
6. Route Change: Wired / Bluetoothの接続・切断で停止と再Open、再生再開、Bluetooth遅延と表示の関係。

## 後続の実装課題

- 停止中・App離脱時もNative Audio Stream自体は維持する現行方針。バックグラウンド消費電力は未測定で、次のDevice評価に含める。Stream停止・Scheduler休止は後続検討。
- Import前の自動Backupは内部保存済みのファイル。保存失敗等で最新Editがまだ内部にない場合は、Import前に現在のProjectを外部Exportする。

- 音色設定は未予約Eventから反映。長いVoiceそのものをリアルタイムで連続Morphする処理は未実装。
- Volume / Muteも未予約Eventに適用。Mute時に現在のTailを即消す専用Mixer経路は後続。
- Mixの±0.8制限は安全用Clampで、製品Master Limiter / Meterではない。最大音量の音質は聴感評価が必要。
- HatのMetal Oscillatorは簡易モデル。Alias / DC / Spectral特性の改善は音色の聴感と測定を合わせて進める。
- 現在の内部Projectは1件。JSON Import / Exportは追加済み。複数Project管理、内部Recovery一覧UI、Sample入りBundleは未実装。保存失敗時の強制終了では直近Editを失う可能性がある。
- Clap / Tom / Rim / Cymbal、Pitch / Gateのより細かな編集、Parameter Lock、Track Lengthはこの安定化後に追加。
- Release署名、配布方法、他Platform、完全Synth / Sampler仕様はまだ未完了。
