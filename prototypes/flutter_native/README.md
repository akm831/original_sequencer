# Flutter + Native Audio Candidate

UI は Flutter、Realtime Audio は Native/C++ に分離する候補です。

- `app`: Flutter UI source
- `native`: Dart FFI へ公開する薄い C ABI / application adapter
- `platform`: Android/iOS audio device adapter

P0 現在は source skeleton のみです。Android runner と FFI wiring は次の checkpoint で追加します。
