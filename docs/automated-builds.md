# Prototype自動Build

`.github/workflows/prototype-ci.yml`は、PrototypeまたはWorkflow変更のpush / Pull Request、および手動実行で起動します。GitHub-hosted Ubuntu 24.04上で実行するため、開発端末のSDK状態には依存しません。

## 実行順序

1. Debug設定でCommon C++ / Flutter C ABIのHost testsをBuildして実行（assertを有効にする）
2. Java 17 / Flutter 3.47.2 / Android SDK 36 / Build-tools 36.0.0 / NDK 28.2.13676358 / CMake 3.22.1を準備
3. `flutter pub get`と`flutter analyze --fatal-infos`
4. `flutter build apk --debug --target-platform android-arm64`
5. APK内のFlutter / C++ Runtime / Native Audio Bridge Libraryを確認
6. APKとCommit / Build種別 / SHA256を記録した`build-info.json`をArtifactとして14日間保存

Flutter候補が最初の自動APK対象です。JUCE AndroidはProjucer exporterの準備が必要なため、このWorkflowにはまだ追加していません。

## APKの入手

GitHub RepositoryのActionsタブで`Prototype tests and Android APK`を開き、成功したRunのArtifactsにある`sequencer-arm64-debug-<commit>`をDownloadしてください。ZIP内の`app-debug.apk`が実機検証用のアプリです。

ARM64 Android端末向けで、Debug署名を使用します。Store公開用のBuildではありません。同じPackage名で異なる署名の既存アプリがある場合、更新Installは失敗することがあります。既存アプリの削除は保存Dataも失うため、確認なしに削除しないでください。

検査 / Build失敗時はArtifactを生成しません。Runの該当Stepに詳細なログが残ります。同じBranchへ新しい変更が入ると、古いRunをCancelして最新の変更を検証します。手動実行ボタンはWorkflowがdefault branchに反映された後に利用できます。

## 実機試験との区別

Build成功とNative LibraryのPackagingまでは自動で確認します。耳での発音、Touch-to-sound Latency、長時間安定性、Route Changeなどの実機P2 / P3比較は別途行います。

初回Run `36962438772`ではC++ tests 4本、Flutter analyze、APK Build、Packaging検査、Artifact Uploadがすべて成功しました。詳細は`docs/prototype-results/flutter-native/ci-build-2026-10-02.md`を参照してください。GitHub ActionsのRepository設定や接続Accountの権限によって実行が止まる場合は、Build成功として扱いません。
