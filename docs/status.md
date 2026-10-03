# Project Status

## Current Topic

Android第一でPlayable Grooveboxを拡張。ユーザーの指示によりJUCE交互比較は保留し、Flutter + C++ / Oboeで進める。最終的な他Platform互換性は後続検証。

## Recently Completed

- Flutter Android P1〜P4の実機発音とUI動作をユーザー確認済み。48 kHz / 96 frames、missedSteps / overflow / restart 0。表示更新は描画同期Ticker、音の予約はNative 5 ms Scheduler / 50 ms Lookahead。
- JUCE P4 ARM64 APKもBuild済み（Run 37127513073）。実機比較は保留、Workflowは手動実行に変更。
- 4 Tracks（Kick / Snare / Hat / Bass）、4 Patterns、各16 Steps、Accent / Volume / Muteを追加。次の未予約小節先頭でPattern切り替え。
- Version付きJSONの自動保存 / 手動保存 / 再起動時復元。AtomicFileで旧保存を保護し、未知Schema / 読み込み失敗では上書き停止。
- Android Version 0.0.3+3。GitHub Actions Run `37130262542`でC++ tests 7本、Flutter analyze、Flutter tests 11本、ARM64 Debug APK / Native Library Packagingすべて成功。Artifact `11276104349`（2026-10-17まで）。PR #6をmain `08c33c0`へ統合済み。新機能の実機確認は未実施。

## Important Decisions

Audio Callbackにallocation / lock / File I/Oを持ち込まない。Core Musical LogicにAndroid APIを持ち込まない。今回の4音源と4×16 Bank Formatは最小演奏Sliceで、製品版約8 Tracks / Engine選択 / Portable Project Formatとは区別する。詳細はdocs/decisions.mdの2026-10-03 Android優先判断。

## Next

新APKで4 Trackの聴感、Pattern切り替え、Volume / Mute / Accent、App再起動後の復元を実機確認。その後SamplerのImport / 発音、Track Engine選択、Note編集、Independent Track Length、Project管理を順に進める。Route Change / 実機長時間負荷は未確認。Web / iOS / DesktopとJUCE再比較はAndroidのPlayable機能を固めてから。

## References

AGENTS.md、docs/decisions.md、docs/android-groovebox.md、docs/roadmap.md、docs/pattern.md、docs/persistence.md。
