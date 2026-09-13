# Project Status

このファイルは、新しいChatGPT / Codex Sessionで現在地を短時間で復元するための「しおり」です。

詳細仕様をここへ複製しません。確定仕様の正本は各`docs/*.md`、判断理由は`docs/decisions.md`です。

## Phase

Specification / Architecture

まだ本格実装開始前です。Framework / Language / UI Toolkit / Audio Backendは未確定です。

## Product Direction

Touch-firstのGroovebox / Sequencerを設計中です。

初期目標は約8 Tracksで、各TrackがSamplerまたはSynth Engineを選択します。Main SequencerはTR-styleの分かりやすい16-step Surfaceを維持しつつ、将来的にParameter Lock、Probability、Micro Timing、Independent Track Rate等へ拡張します。

## Recently Completed

- Sequencer Core Model
- 960 PPQN Timing方針
- Velocity / Accent
- Swing / Micro Timing方針
- Step Resolution / Note Length / Tie / Legato
- NOTE / CHORD / KEYBOARD入力方式
- Sampler基本仕様
- Synth v0.1基本仕様
- Harmony / Chord拡張方針
- Pattern構造 / Pattern切替
- Project Persistence / Schema Versioning
- Sample Asset Import / Portable Asset方針
- Waveform Min / Max Peak Cache方針
- Sampler Playback Voice Behavior
- Sampler Retrigger / Overlap方針
- Sampler Polyphony / Voice Stealing方針
- Choke Group / Pattern切替時のSampler Voice方針
- Common Voice Contract方針
- Audio Engine Scheduling責務分離
- Sample-accurate Scheduling方針
- Scheduler / Audio Callback Queue境界
- Pattern切替のAudio Thread Scheduling方針
- Parameter LockのResolved Trigger State境界
- Audio Buffer / Latency方針
- Sequencer Lookahead / Live Input分離方針
- Queue Capacity / Overflow Safety方針
- Audio Device Restart / Clock再同期方針
- Global Polyphony / Performance Budget方針
- Callback Headroom / Performance Diagnostics方針
- 32 Voice Baseline / 64 Voice Stretch Benchmark方針
- Device Performance Profile方針
- Audio / MIDI基礎Learning Note

## Current Topic

Platform / Framework評価に必要なAudio Requirements

次に検討する主題:

- Audio Backend候補が満たすべきRealtime要件
- Sample-accurate Event Scheduling対応
- Variable Buffer Size / 128 Frames Target対応
- Low-latency Live Input経路
- Realtime-safe Queue / Threading要件
- Sampler Streaming / Asset DecodeのThread境界
- 32 Voice Baseline / 64 Voice Stretch Benchmarkの実施可能性
- iOS / Android / Desktopを見据えたAudio Session / Device Restart対応
- Flutter / Native / JUCE / Web系構成を比較するための評価軸

Audio Schedulingの正本は`docs/audio-engine.md`、Buffer / Latencyは`docs/audio-buffer-latency.md`、Performance Budgetは`docs/performance-budget.md`です。

## Important Current Decisions

- SequencerとAudio Engineを分離する
- TrackがSampler / Synth等のEngineを選択する
- v0.1 Main Gridは1 Step = 1/16 note
- Timing内部目標は960 PPQN
- Pattern切替は演奏中なら次のProject bar boundary
- Patternは現仕様ではTracks / Engine State / Mixer Stateを所有する
- Project BPMはGlobal
- Imported Sampleは原則Project Assetへ取り込む
- Samplerは絶対PathではなくStable Asset IDを参照する
- Sample編集は原則非破壊
- Waveform表示には再生成可能なMin / Max Peak Cacheを使う
- Project保存には明示的なschemaVersionを持たせる
- Preset名だけでなく実際のEngine StateをProjectへ保存する
- Missing SampleがあってもProject全体は可能な限り開く
- Sampler Triggerは独立Voiceを生成し、同一Sample Retriggerは標準でOverlapを許可する
- Samplerは1 Trackあたり最大8 Voicesを暫定上限とする
- Voice StealingはRelease中の最古Voiceを優先し、その後は最古Voiceを選ぶ
- Choke GroupはTrackをまたいでSampler Voiceへ適用できる
- Pattern切替ではSampler Gate / LoopをReleaseへ移行し、One Shot Tailは原則Carryする
- Sampler VoiceはTrigger時点の有効なEngine Stateを保持する
- SchedulerとAudio Callbackを分離し、LookaheadでTimestamp付きAudio Commandを準備する
- Buffer内Eventは可能な限りSample Offset位置で実行する
- Audio ThreadはUI / Project Model / File I/Oへ直接依存しない
- Parameter LockはAudio Thread外でEvent-localなResolved Stateへ解決する
- ChokeはVoice Allocation前、Voice Stealingは必要時のAllocation Fallbackとして処理する
- Common Voice ContractはLifecycleだけを共有し、Sampler / Synth固有DSP Stateは各Engine内部に保持する
- Synth Polyも1 Trackあたり最大8 Voicesを暫定上限とする
- Audio Logicを特定Buffer Sizeへ固定しない
- 128 Frames程度をPreferred Target、256 Frames程度をStable Fallbackとする
- 512 Frames以上でもCompatibility動作できる構造を維持する
- Sequencer Lookaheadは約50 msを初期候補とするが、Live InputはそのLookaheadを待たない
- Scheduler → Audio Thread QueueはBoundedとし、通常再生で十分なHeadroomを持つ
- Queue OverflowでもAudio ThreadをBlockせず、Stop / Panic系を失いにくいFail-safeを持つ
- Device Restart時はPending CommandとAudio Frame Originを再構築する
- Project Musical TimeはSample Rate変更から独立させる
- Track Voice LimitとGlobal Safety Budgetを分離する
- Engine構造は最大64 Active Voiceを表現可能にする
- 32 Concurrent VoicesをBaseline Performance Target候補とする
- 64 Light-to-Moderate VoicesをStretch Targetとする
- Reference ProjectではCallback Load約50%以下をNormal Targetとする
- 継続的に70〜80%以上へ張り付く状態はHeadroom不足として評価する
- Global Budget到達はSafety Fallbackとし、通常時に頻発させない
- Dynamic Quality Scalingはv0.1必須にしない
- Device / Backend Performance ProfileはProjectへ保存しない

## Primary References

現在のTopicを再開するときは、まず以下を確認します。

1. `AGENTS.md`
2. `docs/status.md`
3. `docs/performance-budget.md`
4. `docs/audio-buffer-latency.md`
5. `docs/audio-engine.md`
6. `docs/architecture.md`
7. `docs/decisions.md`

必要になった場合のみ、関連する詳細仕様を追加で読みます。

全仕様を毎Session最初から要約し直す必要はありません。

## Learning Notes

基礎知識は仕様書へ混ぜず、`docs/learning/`へ蓄積します。

現在:

- `docs/learning/audio-midi-basics.md`

今後、Audio Buffer / Latency、Bit Depth、dB、ADSR、Filter、Polyphony / Voice、PPQN等を必要に応じて追加します。

## Session Handoff Policy

Chatが長くなった場合、巨大な引き継ぎPromptを作成しません。

Session終了前に行うこと:

1. 確定した仕様を該当する`docs/*.md`へ反映する
2. 重要な判断を`docs/decisions.md`へ反映する
3. 必要な基礎知識を`docs/learning/`へ反映する
4. この`docs/status.md`のRecently Completed / Current Topic / Nextを更新する

新しいSessionでは、原則としてGitHubをSource of Truthとして復元します。

推奨する短い再開指示:

```text
original_sequencerの続きを進めてください。AGENTS.mdとdocs/status.mdを確認し、GitHubの仕様を正本として現在地から再開してください。
```

長大な会話履歴や手作業の引き継ぎSummaryへ依存しないことを原則とします。

## Next

Platform / Framework評価に必要なAudio Requirementsを仕様化する。

その後の有力候補:

- Framework / Language / Audio Backend候補比較
- Sample Streaming / Caching
- Resampling / Interpolation Quality
- Effect Architecture / CPU Budget
