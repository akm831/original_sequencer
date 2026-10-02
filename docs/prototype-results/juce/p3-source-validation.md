# JUCE P2 / P3 Source Validation — 2026-10-02

- Scheduled TriggerボタンはFlutterと同じ`ScheduledTriggerInput`を使用
- Common CoreのPlanar出力でJUCE Bufferの対象区間へ直接Render。CallbackでBuffer変換用Memoryを確保しない
- Callback Timing / Load / Percentile / Queue / Trigger Diagnosticsを表示
- 連続P1 Test Toneは停止し、同じ50 msのMonophonic検証用Burstへ移行
- UI入力とCore LifecycleはControl側Mutexで直列化。通常Audio CallbackはMutex不使用

検証:

- JUCE 9.0.2（commit `7278278`）公式Headerに対し、GCC C++20 / `-Wall -Wextra -Werror` / `-fsyntax-only`でMain.cppが成功
- `audio_trigger_smoke`でPlanarとInterleavedのOutput Sampleが完全一致。start offset前後のSampleは保持し、Null Channelを安全にSkip

未検証:

JUCE appのLink / Android build / GUIの実行・Layout / Audio Deviceの実行・Restart / 実機P2・P3。HostとHeaderの検証は実機比較の代替ではない。
