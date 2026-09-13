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
- Audio / MIDI基礎Learning Note

## Current Topic

Global Polyphony / Performance Budget

次に検討する主題:

- Global Voice Budgetの役割と初期Target
- Sampler / Synth / Mixer / FXごとのCPU Budget
- Track単位Voice LimitとGlobal Budgetの関係
- Global Budget到達時のVoice Stealing / Degradation Policy
- Callback Deadlineに対するSafety Margin
- Performance Diagnosticsの合格基準
- Framework / Audio Backend比較で測るBenchmark項目

Audio Schedulingの正本は`docs/audio-engine.md`、Buffer / Latencyの正本は`docs/audio-buffer-latency.md`です。

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
- Synth Polyも1 Trackあたり最大8 Voicesを暫定上限とし、Global Voice Budgetの具体値は実機計測後に決定する
- Audio Logicを特定Buffer Sizeへ固定しない
- 128 Frames程度をPreferred Target、256 Frames程度をStable Fallbackとする
- 512 Frames以上でもCompatibility動作できる構造を維持する
- Sequencer Lookaheadは約50 msを初期候補とするが、Live InputはそのLookaheadを待たない
- Scheduler → Audio Thread QueueはBoundedとし、通常再生で十分なHeadroomを持つ
- Queue OverflowでもAudio ThreadをBlockせず、Stop / Panic系を失いにくいFail-safeを持つ
- Device Restart時はPending CommandとAudio Frame Originを再構築する
- Project Musical TimeはSample Rate変更から独立させる

## Primary References

現在のTopicを再開するときは、まず以下を確認します。

1. `AGENTS.md`
2. `docs/status.md`
3. `docs/audio-buffer-latency.md`
4. `docs/audio-engine.md`
5. `docs/voice-management.md`
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

Global Polyphony / Performance Budgetを仕様化する。

その後の有力候補:

- Platform / Framework評価に必要なAudio要件整理
- Framework / Language / Audio Backend候補比較
- Sample Streaming / Caching
- Resampling / Interpolation Quality
