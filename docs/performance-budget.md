# Global Polyphony / Performance Budget仕様

## 目的

Sampler / Synth / Mixer / FXを同時に動作させたとき、Audio CallbackのDeadlineを守りながら音楽的に十分なPolyphonyを提供するためのPerformance Budgetを定義します。

この文書では「Voice数」だけを性能の尺度にしません。

同じ1 Voiceでも、短いPCM Sample再生と、2 OSC + Filter + Envelopeを持つSynthではCPU Costが異なるためです。

## 基本原則

- Track / EngineごとのMusical Voice Limitと、Audio Engine全体のSafety Budgetを分離する
- Voice Countは分かりやすい上限制御として使う
- 実際の性能判定はAudio Callbackの処理時間、Underrun、CPU Headroomを重視する
- Global Budget到達は通常状態ではなくSafety Fallbackとして扱う
- Device性能によって演奏中に無秩序にPolyphony上限を上下させない
- Audio Thread上で高コストな負荷分析や動的最適化を行わない
- 性能不足時は音切れよりも予測可能なDegradationを優先する
- 最終的な数値はReference Benchmarkと実機計測で決定する

## 3段階のBudget

Performance制御は次の3段階で考えます。

```text
Track / Engine Voice Limit
          ↓
Global Voice Safety Limit
          ↓
Audio Callback CPU / Deadline Budget
```

### 1. Track / Engine Voice Limit

音楽的な挙動を決める上限です。

現時点のv0.1暫定値:

- Sampler Track: 最大8 Voices
- Synth Poly Track: 最大8 Voices
- Synth Mono Track: 1 Voice

Track Limitへ到達した場合は、そのTrack / Engine内でVoice Stealingを行います。

既定Policy:

1. Release中の最古Voice
2. それ以外の最古Voice

SamplerではすでにこのRuleを採用しています。

### 2. Global Voice Safety Limit

Audio Engine全体のActive Voice数に対するSafety Capです。

初期構成が約8 Tracksで各Track最大8 Voicesなので、理論上の上端は概ね64 Voicesです。

したがってv0.1のEngine構造は、少なくとも**最大64 Active Voiceを表現可能**にします。

ただし、これは「どの端末でも重いSynth Voiceを64音必ず同時発音できる」という性能保証ではありません。

Global Safety Limitの実際の有効値は、Backend / Device Capability Profileで決定できる構造にします。

重要:

- Project Data自体へDevice固有のGlobal Limitを保存しない
- Device起動時またはAudio Backend初期化時にCapabilityを決める
- 演奏中に負荷を見ながら毎瞬間Limitを上下させるAdaptive Polyphonyはv0.1では行わない

### 3. Callback Performance Budget

最も重要な実性能指標です。

Audio Callbackは次回Deadlineまでに必ず処理を終える必要があります。

概念:

```text
Buffer Duration
|------------------------------|

Audio Processing
|-------------|

残り = Safety Headroom
```

CPU使用率をOS全体のCPU%だけで判断せず、Callbackごとに

```text
callbackLoad
= callbackProcessingTime / bufferDuration
```

を計測できるようにします。

## Callback Headroom目標

Prototype / Backend比較時の初期Performance Targetとして以下を採用します。

### Normal Target

- 通常のReference ProjectでCallback Loadが概ね50%以下に収まることを目標とする

### Peak Target

- 一時的なPeakでも、継続的に70〜80%付近へ張り付かないことを目標とする

### Failure Condition

- Callback Deadline超過
- Underrun / Dropout
- 継続的な90〜100%近辺のCallback Load

はPerformance不足として扱います。

50%は固定の製品保証値ではなく、UI / OS Interrupt /一時的な負荷変動へ耐えるHeadroomを残すための設計目標です。

## v0.1 Reference Polyphony Target

Framework / Audio Backend比較時に、最低限の共通Benchmarkを持ちます。

### Baseline Target

**32 Concurrent Voices**を、48 kHz / Preferred 128 Frames程度の条件で安定処理できることを最低Performance Target候補とします。

32 Voicesの理由:

- 8 Tracks × 平均4 Voicesという実用的なChord / Tail構成を想定できる
- 初期Harmonyの3〜4音Chordを複数Trackで扱える
- Sampler TailとSynth ReleaseのOverlapをある程度含められる
- Mobile向けv0.1として現実的な基準を作りやすい

### Stretch Target

**64 Light-to-Moderate Voices**を安定処理できることを望ましいStretch Targetとします。

ただし64 Voicesを重いSynth Patchだけで構成した場合まで保証するものではありません。

Voice Countだけでなく、Sampler / Synthの混在Scenarioで測定します。

## Reference Benchmark Scenarios

Backend / Framework比較では少なくとも以下を用意します。

### Scenario A: Sampler Heavy

- 8 Tracks
- 複数One Shot Tail
- 合計32 Voices前後
- Pitch変換あり / なしの両方を確認

目的:

- Sample Playback
- Voice Mixing
- Memory Access
- Resampling Cost

を確認します。

### Scenario B: Synth Heavy

- 複数Synth Tracks
- 2 OSC + Noise
- Filter
- Amp / Filter Envelope
- 合計16〜32 Synth Voices

目的:

- Oscillator
- Filter
- Envelope
- Polyphonic DSP

の負荷を確認します。

### Scenario C: Mixed Groovebox

実使用に最も近い基準です。

例:

- Drum Sampler Tracks
- Bass Synth
- Chord / Pad Synth
- FX / Vocal Sample
- Mixer Level / Pan
- Parameter Lock
- Pattern Switching

合計Active Voice数が時間とともに変化するProjectを数分以上Loop再生します。

### Scenario D: Stress / Burst

- Pattern Boundaryで複数Trackが同時Trigger
- Chord + Release TailのOverlap
- Parameter Lockを伴うTrigger Batch
- Live Inputを追加

瞬間的なPeakでDeadlineを超えないか確認します。

## Benchmark時間

一瞬だけ正常再生できることでは合格としません。

Prototypeでは少なくとも数分間の連続Loop Testを行い、正式なBackend候補比較では**10分程度のStress Playback**を基準候補とします。

計測項目:

- Callback平均処理時間
- Callback Peak / Percentile Load
- Underrun / Dropout Count
- Active Voice Count / Peak
- Voice Steal Count
- Queue High-water Mark
- Queue Overflow Count
- Thermal / Performance変化が観測可能ならその傾向

## SamplerとSynthを同じCostとして扱わない

Global Budgetを単純な「64ポイント制」のような固定Weight Modelには、v0.1ではしません。

理由:

- Sample Pitch / Interpolation方式
- Synth Filter Algorithm
- Sample Rate
- Effect構成
- Device SIMD / CPU特性

によってCostが変わるためです。

初期実装は

- Voice Count Limit
- Callback Load計測
- Reference Benchmark

の3つを組み合わせます。

将来、必要になればEngineごとのCost WeightやDynamic Quality Scalingを検討します。

## Mixer / FX Budget

MixerとEffectはVoiceとは別にCPUを消費します。

したがってVoice処理でCallback Deadlineを使い切ってはいけません。

v0.1では具体的なFX構成が未確定なため、Voice EngineだけでBenchmark時のCPU Budgetを100%使う設計にはしません。

Normal TargetをCallback Load約50%以下とするのは、将来の

- Mixer
- Master Processing
- Delay / Reverb
- UI / OS負荷変動

などへHeadroomを残す意味もあります。

Effect Architecture決定後、必要なら専用Budgetを追加します。

## Global Budget到達時の処理

Global Safety Limitへ到達した場合、Audio ThreadをBlockしたりMemoryを無制限に増やしたりしません。

処理順:

1. Finished Voiceを回収する
2. Choke対象Voiceを終了する
3. Trigger対象Track内のVoice Limit / Stealingを適用する
4. それでもGlobal Slotが不足する場合にGlobal Fallbackを適用する

Global Fallbackは例外的なSafety処理とします。

### Global Steal Priority

v0.1候補:

1. Release中の最古Voice
2. 既にStopping状態のVoice
3. それでも不足する場合は最古のActive Voice

強制終了時は短いDe-click Fadeを使用します。

ただし、Global Stealが通常再生で頻発する設定は「正常」と扱わず、Capability ProfileまたはPerformance要件を見直します。

## Live Inputの扱い

Userが今TapしたLive Noteを、古いRelease Tailより低Priorityにして無音にすることは避けます。

Global Slot不足時は、可能な限り既存のRelease /古いVoiceを整理してLive Input用Slotを確保します。

ただしAudio Thread上で複雑なPriority Score計算は行いません。

v0.1では単純でDeterministicなRuleを優先します。

## Quality Degradation Policy

負荷が高くなった瞬間に、Synth Quality、Filter Quality、Sample Interpolation Quality等を自動的に切り替えるDynamic Quality Scalingはv0.1必須にはしません。

理由:

- 音色が演奏中に突然変化する
- Testが難しくなる
- Backendごとの挙動差が増える

最初は

1. 十分なHeadroomを持つ
2. Track / Global Voice Limitで上限を制御する
3. 必要ならBuffer Sizeを安全側へFallbackする

という予測可能な方式を優先します。

将来、Desktop / Mobile各Platformで必要になればQuality Tierを追加できます。

## Device Capability Profile

Audio Backend初期化時に、Device / Platformごとの安全なPerformance Profileを選択できる構造にします。

概念例:

```text
AudioPerformanceProfile
├─ preferredBufferFrames
├─ globalVoiceLimit
├─ sampleRate
└─ future: qualityTier
```

ただしUser ProjectへこのProfileを保存しません。

同じProjectを別Deviceで開いても、音楽データそのものは変化しないようにします。

## Performance不足時の優先順位

品質劣化への対応優先順位は次を基本とします。

1. Realtime Safetyを守る
2. Stuck Voice / Crashを防ぐ
3. Timingを守る
4. 発音数を必要に応じ制限する
5. Latencyを少し増やすFallbackを許容する
6. 音質Algorithmの自動変更は最後の手段とする

つまり、少しLatencyが増えても安定再生する方を、低LatencyのままClick / Dropoutする状態より優先します。

## Diagnostics

Development Buildでは最低限以下を観測可能にします。

- Current Active Voices
- Peak Active Voices
- Voices per Track / Engine
- Voice Steal Count
- Global Steal Count
- Callback Average Load
- Callback Peak Load
- Callback Percentile Load
- Underrun / Dropout Count
- Buffer Frames
- Sample Rate
- Queue High-water Mark

これらはFramework / Backend比較の共通指標として使用します。

## v0.1で確定するScope

- Track Voice LimitとGlobal Safety Budgetを分離する
- Sampler / Synth Polyは各Track最大8 Voicesを暫定上限とする
- Engine構造は最大64 Active Voiceを表現可能にする
- 64 Voicesを全Device / 全Patchで保証するとはしない
- 32 Concurrent VoicesをBaseline Performance Target候補とする
- 64 Light-to-Moderate VoicesをStretch Targetとする
- Voice CountだけでなくCallback Loadを主要Performance指標とする
- Reference ProjectではCallback Load約50%以下をNormal Targetとする
- 継続的に70〜80%以上へ張り付く状態はHeadroom不足として評価する
- Global Budget到達は通常BehaviorではなくSafety Fallbackとする
- Global StealはRelease中の最古Voiceを優先する
- Live Inputは古いRelease Tailより優先してSlot確保を試みる
- v0.1ではDynamic Quality Scalingを必須にしない
- Performance ProfileはDevice / Backend側で決め、Projectへ保存しない
- Backend比較ではSampler / Synth / Mixed / Stressの共通Benchmarkを使う

## 次の検討

- Platform / Framework評価に必要なAudio Requirements整理
- Framework / Language / Audio Backend候補比較
- Sample Streaming / Caching
- Resampling / Interpolation Quality
- Effect Architecture / Effect CPU Budget
- Synth Mono Legato / Retrigger Semantics
