# Project Status

## Current Topic

808系Percussion / 303系Bassの音源と編集をAndroid優先で拡張。ユーザーの指示によりJUCE交互比較は保留し、Flutter + C++ / Oboeで進める。最終的な他Platform互換性は後続検証。

## Recently Completed

- Flutter Android P1〜P4の実機発音とUI動作をユーザー確認済み。48 kHz / 96 frames、missedSteps / overflow / restart 0。表示更新は描画同期Ticker、音の予約はNative 5 ms Scheduler / 50 ms Lookahead。
- JUCE P4 ARM64 APKもBuild済み（Run 37127513073）。実機比較は保留、Workflowは手動実行に変更。
- 4 Tracks（Kick / Snare / Hat / Bass）、4 Patterns、各16 Steps、Accent / Volume / Muteを追加。次の未予約小節先頭でPattern切り替え。
- Version付きJSONの自動保存 / 手動保存 / 再起動時復元。AtomicFileで旧保存を保護し、未知Schema / 読み込み失敗では上書き停止。
- Android Version 0.0.3+3。GitHub Actions Run `37130262542`でC++ tests 7本、Flutter analyze、Flutter tests 11本、ARM64 Debug APK / Native Library Packagingすべて成功。Artifact `11276104349`（2026-10-17まで）。PR #6をmain `08c33c0`へ統合済み。ユーザーが実機で「かなりよく動く」と確認。診断画像はmissedSteps / Overflow / Restart 0、48 kHz / 96 frames、p99 5%、Peak 41.32%。保存復元の個別確認は未報告。

- Version 0.0.4+4: Analog Percussion、Saw / Square Acid Bass、Pitch / Tone / Decay / Cutoff / Resonance / Filter Envelope、Bass Note / Accent / Slide、Hat開閉、Schema 1→2 Migrationを実装。Run `37132997199`でC++ tests 8本 / Flutter tests 14本 / analyze / ARM64 APK / Exported FFI Symbols検査が成功。PR #7をmain `b85aae8`へ統合。Artifact `11277543381`。音色の実機確認は未実施。

- Version 0.0.5+5: 一括Track更新、Stop Fade / Retrigger smoothing、密な音源Stress Test、文字拡大Grid、Play Bank整合、保存Read上限、Audio Focus / 非同期Grant取消を追加。公開テスト用署名の固定と証明書検査、JSON Import / Export、読めない保存のRaw Exportを追加。CI結果は最新PR参照。懸念点の台帳はdocs/android-audio-review.md、初回署名移行はdocs/android-update-backup.md。

## Important Decisions

Audio Callbackにallocation / lock / File I/Oを持ち込まない。Core Musical LogicにAndroid APIを持ち込まない。今回の4音源と4×16 Bank Formatは最小演奏Sliceで、製品版約8 Tracks / Engine選択 / Portable Project Formatとは区別する。詳細はdocs/decisions.mdの2026-10-03 Android優先判断。

## Next

新APKで808系Percussion / Acid Bassの聴感と調整幅、Bass Note / Slide / Accent、旧保存の復元を実機確認。以後音色の改善、Clap / Tom / Rim等の追加、Parameter Lock、Track Lengthへ進む。Sampler / Platform互換性の順番は必要に応じて後続。Route Change / 実機長時間負荷は未確認。

## References

AGENTS.md、docs/decisions.md、docs/android-groovebox.md、docs/roadmap.md、docs/pattern.md、docs/persistence.md、docs/android-audio-review.md。
