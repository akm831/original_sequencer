# P3 Scheduled Trigger Host Verification — 2026-10-02

対象: Common C++ Core / Flutter C ABI / Android Adapter source。
Android実機でのP3結果ではありません。

## 実装

- 指定Sample Offsetから50 ms / 220 Hz / 最大Amplitude 0.08の検証用BurstをRender
- FlutterのScheduled TriggerボタンとQueue / Trigger Diagnostics
- Control側でCommand投入とLifecycleを直列化。Audio CallbackはMutex不使用
- 従来の連続Test ToneはFlutter側で停止。JUCEは未変更

## 成功した検証

GCC C++20、`-Wall -Wextra -Werror -pthread`で直接Buildして実行:

- `audio_command_queue_smoke`: Capacity / FIFO / Overflowと10万件の並行転送
- `audio_core_smoke`: Offset / Callback境界 / Variable Buffer / Restart Pending破棄 / 並行Diagnostics
- `audio_trigger_smoke`: Output SampleでOffset 37 / 95、次Callback Offset 0、Stereo一致、Late / Same-time Trigger、Reset、Burst終了、4096 Framesを異なる区切りでRenderして完全に同じ波形になること
- `flutter_bridge_smoke`: C ABI投入から実出力まで、Queue満杯、入力Validation、停止中拒否、再初期化、Control側Lifecycleと投入・診断の並行呼び出し

追加:

- UndefinedBehaviorSanitizer: `audio_trigger_smoke`成功
- ThreadSanitizer: `audio_core_smoke` / `flutter_bridge_smoke`でData Race報告なし
- Oboe 1.10.0（tag commit `a81bb9f`）の公式Headerを使い、`-D__ANDROID__ -fsyntax-only`でAndroid backend / bridgeの構文検査成功
- C / Dart Diagnosticsの17 Fieldsが型・順序で一致することをSource比較で確認

## 未検証 / 環境制限

- CMake / Flutter SDK / Android NDKはこの環境にない。CMakeによるBuild、`flutter analyze`、APK Build、Dart FFIの実行、実機発音・Route Changeは未実施
- Header構文検査はAndroid ToolchainでのLink / APK Packagingの検証ではない
- AddressSanitizerは実行環境の`/proc`制限によりLeakSanitizerが終了できず、成功扱いにしていない
- Monophonic BurstはP3の検証音。Poly Voice、Live Pad、End-to-end Latency、P3負荷baselineは未確認

最終Technology DecisionやP3実機完了の根拠としてこのHost結果だけを使わない。
