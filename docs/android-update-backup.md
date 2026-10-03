# Android APK更新とバックアップ

## 署名の切り替え

0.0.5+5の安定化版から、CIとMacのDebug Buildで同じ公開テスト用署名を利用する。以後この署名のAPK同士は通常の更新Installで内部保存を維持できる。製品Release用の鍵ではない。

以前のCI APKやMac独自のDebug署名とは一致しない可能性がある。Androidが「署名が異なる」「パッケージが競合する」と拒否した場合、保存データを残したいなら先にアンインストールしない。既存版の署名が合わない問題を、新APK側だけで解消することはできない。

## 既存版から初回移行する場合

現在のアプリにファイルExportがあるなら、プロジェクトメニューから書き出す。ない場合はMacとUSB Debugで接続し、旧Debugアプリの内部JSONをMacへ退避する。

```bash
adb devices
adb exec-out run-as com.originalsequencer.prototype cat files/groove-project.json > groove-project-backup.json
python3 -m json.tool groove-project-backup.json
```

最後のコマンドでJSONを正常表示できることを確認する。run-as失敗や空ファイルならバックアップできていないので、アンインストールを進めない。まだ保存したことがなくファイルが存在しない場合は、現在のアプリで保存ボタンを押してから再試行する。

バックアップが取れていて移行する場合だけ、署名が合わない旧版を削除して新APKをInstallする。これはユーザーが端末で行う操作で、自動削除は行わない。JSONを端末のFiles等へ移し、新版の「ファイルから読み込む」で選択する。Schema 1は自動移行される。

## 新版での通常バックアップ

右上のプロジェクトメニュー →「ファイルへ書き出す」。Androidのファイル選択画面で保存先を指定する。再生は停止し、最新の編集状態をJSONへ書き出す。

「ファイルから読み込む」はJSONを検査した後、置き換え確認を表示する。読み込み前の内部保存を`groove-recovery-*.json`として保持し、Backupに失敗すれば置き換えない。読み込み後は内部保存し、再生は停止状態。未知Schemaは拒否するが、読めない元ファイル自体は書き出して保管できる。

外部ファイルへの書き込みはAndroid Document Providerが担当し、キャンセル／クラウド保存先の失敗は通知する。内部自動保存のAtomicFileとは別の経路なので、保存先サービスの障害までAtomic保証はしない。
