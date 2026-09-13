# Android Audio Adapter

P1 で Oboe/AAudio を使う low-latency output callback を追加します。

Callback 内では Flutter/Dart を呼ばず、Reference Audio Core の `render()` のみを realtime path から呼ぶ構造にします。
