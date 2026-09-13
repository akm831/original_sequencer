# Technology Prototypes

Flutter + Native/C++ Audio と JUCE/C++ を、同じ Reference Audio Core と評価条件で比較するための実験領域です。

## 現在の到達点

P0 の最初の実装単位として、`common/audio_core` を Framework 非依存の C++20 static library としてビルドできる骨格を追加しています。

```bash
cmake -S prototypes -B build/prototypes
cmake --build build/prototypes
ctest --test-dir build/prototypes --output-on-failure
```

現時点の Core は silence render と最小 lifecycle/diagnostics のみです。P1 の実機 Audio Callback、Sine test、actual sample rate / callback frames の取得はまだ完了していません。

## Directory

- `common/audio_core`: 候補間で共有する Reference Audio Core
- `common/benchmark`: 共通 benchmark 定義
- `common/test_assets`: 共通 test asset
- `flutter_native`: Flutter UI + Native/C++ Audio 候補
- `juce`: JUCE/C++ 候補

仕様の正本は `docs/technology-prototype.md` と `docs/prototype-implementation-plan.md` です。
