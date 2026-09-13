# Audio Buffer / Latency仕様

## 目的

Audio Engineが低Latencyと安定再生を両立するための、Audio Buffer Size、Scheduler Lookahead、Queue容量、Device Restart時の基本方針を定義します。

この文書では特定のFramework / Audio Backendへ依存するAPI値は固定せず、Backend選定時に満たすべき製品要件を定義します。

## 基本原則

- Buffer Sizeは小さいほど反応は速くなるが、CPU Deadlineは厳しくなる
- Buffer Sizeを大きくするとDropout耐性は上がるが、Latencyは増えやすい
- Sequencer EventのLookahead SchedulingとAudio I/O Buffer Sizeは別概念として扱う
- Sequencer Eventは先読みしても、TimestampどおりのAudio Frameで発音する
- Live Touch / Keyboard入力は長いSequencer Lookaheadを待たせず、可能な限り次の安全なAudio処理機会へ渡す
- Audio ThreadはQueue不足やUI処理を待ってBlockしない
- 具体的な最終値は実機計測で決定する

## Audio Buffer Sizeとは

Audio Deviceは通常、1 Sampleずつではなく一定数のFramesをまとめてAudio Callbackへ要求します。

概念:

```text
Audio Device
    ↓
[128 frames]
    ↓
Audio Callback
    ↓
[128 frames]
    ↓
Audio Callback
    ...
```

48 kHzの場合の1 Buffer分の時間は概算で以下です。

```text
64 frames  ≒ 1.33 ms
128 frames ≒ 2.67 ms
256 frames ≒ 5.33 ms
512 frames ≒ 10.67 ms
```

ただし、これはBuffer 1個分の時間であり、Userが感じる最終的なOutput Latencyそのものではありません。

実際のLatencyにはOS / Driver / Hardware / Internal Queue等も影響します。

## v0.1のBuffer Target

特定Backendを選ぶ前の性能目標として、以下を採用します。

### Preferred

- 128 Frames程度で安定動作できることを目標にする

### Acceptable Stable Fallback

- 256 Frames程度でも正常動作すること

### Degraded / Compatibility Fallback

- 512 Frames以上を必要とするDeviceでも可能な限り動作は継続する
- ただしLive演奏用途ではLatencyが目立つ可能性があるため、理想状態とは扱わない

64 Frames以下は将来の低Latency最適化候補ですが、v0.1の必須条件にはしません。

重要:

- Application内部のLogicを128 Frames固定にしない
- Device / Backendから得た実際のCallback Frame Countを扱えるようにする
- DeviceによってCallback Sizeが可変でも破綻しない構造を優先する

## Target Latency

現段階では「総Latencyを必ずX ms以下」といった保証値は固定しません。

理由:

- OS
- Device
- Audio Backend
- Bluetooth等の出力経路
- Sample Rate
- Buffer Size

によって大きく変化するためです。

ただし製品目標として、内蔵Speaker / 有線Audio等のLow-latency Routeでは、Touch演奏やKeyboard演奏が楽器として不自然に感じにくい応答性を優先します。

Backend比較時には少なくとも以下を実測します。

- Tap / Note OnからAudio Output開始までのLatency
- Sequencer Event Timing Jitter
- Dropout / Underrun発生率
- CPU負荷増加時の安定性

Bluetooth AudioはCodec / OS Bufferによる追加Latencyが大きい場合があるため、Engineだけで同等Latencyを保証しません。

## Sequencer Lookahead

Lookahead Schedulerは、現在位置より少し先のSequencer Eventを事前にTimestamp付きCommandへ変換します。

初期Targetとして、**約50 ms前後のLookahead**を候補とします。

ただし固定値ではなく、Backend / Device検証で調整可能にします。

概念:

```text
現在                         50 ms先
|------------------------------|
      Schedulerが準備
```

Lookaheadを持つ理由:

- UI Threadの瞬間的な負荷変動からAudio Schedulingを守る
- Step / Pattern / Parameter Lockの評価をAudio Thread外で済ませる
- Audio Callback直前に重いSequencer計算をしない

Lookaheadが50 msあっても、Eventを50 ms早く鳴らすわけではありません。

EventはTarget Frameを保持し、その時刻にAudio Callback内で実行します。

## LookaheadとLive Inputを分離する

Touch Keyboard、Pad Audition等のLive Inputを、通常の50 ms Sequencer Lookaheadの末尾へ配置してはいけません。

Live Inputは低Latency Commandとして扱い、可能な限り次の安全なAudio Buffer / Sample位置で実行します。

概念:

```text
Sequencer Event
    → Lookahead Scheduler
    → Future Timestamp

Live Tap / Keyboard
    → Low-latency Command Path
    → Earliest Safe Audio Time
```

両者は最終的には同じVoice / Engineへ到達できますが、Scheduling Policyを分けます。

これによりSequencerの安定性のための先読みが、Live演奏の体感Latencyを直接増やすことを避けます。

## Project編集とLookahead Window

Schedulerが既に未来EventをQueueへ入れた後にUserがStepを編集した場合、変更がLookahead Window内の既Scheduled Eventへ即座に反映されない可能性があります。

v0.1では、複雑なCommand取消機構を必須にしません。

基本方針:

- 未Schedulingの未来Eventには最新Project Stateを使う
- 既にAudio Queueへ確定済みのごく近いEventは、そのまま実行されることを許容する
- Lookaheadを必要以上に長くしない

これによりRealtime Safetyと編集反応性を両立します。

将来、Live Editing精度を高める必要があればEvent Revision / Cancellation IDを追加できます。

## Safety Margin

SchedulerはAudio Callbackの直前ぎりぎりまでEvent生成を待たず、常に一定のFuture Coverageを維持します。

初期方針:

- Lookahead Target: 約50 ms
- Minimum Safety Coverage: 少なくとも複数Audio Buffers分を維持する
- Scheduler Wake-upの一時的な遅れが1回発生しても、直ちにAudio Event不足にならない設計にする

具体的なScheduler更新周期はFramework / Threading Model選定後に決定します。

## Underrun / Dropout

### Audio Underrun

Audio Deviceが次のBufferを必要としているのに、Audio CallbackがDeadlineまでに処理を完了できない状態です。

結果:

- Click
- Crackle
- Audio Gap
- Dropout

等が発生する可能性があります。

### 優先方針

Audio Threadでは以下を避けます。

- Blocking Lock
- File I/O
- Network I/O
- UI更新
- Project全体の探索
- 不要なMemory Allocation
- Log大量出力

負荷不足が発生するDeviceでは、まず音切れを防ぐためBuffer Sizeを大きくするFallbackを許容します。

「常に最小Buffer」を品質目標にはしません。

## Queue容量

Scheduler → Audio Thread QueueはBounded Capacityを持つ前提とします。

無制限にMemoryを増やすQueueにはしません。

初期実装Targetとして、**少なくとも1024〜2048 Audio Commands程度を保持できる容量**を検討します。

最終値は以下を使って実機検証します。

- 約8 Tracks
- Polyphonic Chord
- Pattern Boundary
- Note On / Off
- Parameter Lock
- 将来のRepeat / Ratchet
- 約50 ms Lookahead

通常再生でQueue使用率が上限へ近づかない十分なHeadroomを持たせます。

## Queue Overflow Policy

Audio ThreadはQueue Overflow解消を待ってBlockしません。

Scheduler側でもOverflowを正常な制御手段として使用せず、通常時に発生しない容量設計を基本とします。

Overflowが起きた場合:

1. Diagnostic Counterへ記録する
2. Audio Threadは処理を継続する
3. Future Trigger等の低Priority Commandより、停止系Commandを優先する
4. Panic / Emergency Stopは通常Queue飽和だけで失われないFail-safe経路を持てる設計とする

Note Off / Transport Stop / PanicがQueue飽和で永久に失われ、Stuck Voiceが残る設計は避けます。

具体的なReserved Slot / Separate Control Flag等の実装方式はBackend選定後に決定します。

## Audio Device Start

Audio Device開始時は、Musical Transport TimeとAudio Frame Timeの基準を明示的に対応付けます。

概念:

```text
transportTickOrigin
↕
audioFrameOrigin
```

Schedulerはこの基準を使ってMusical TickをTarget Frameへ変換します。

Application起動時のWall ClockだけをAudio Timingの正本にしません。

## Audio Device Restart / Route Change

Mobileでは以下が起こり得ます。

- Audio Route変更
- Headphone接続 / 切断
- Audio Session interruption
- Sample Rate変更
- Device Callback再初期化

この場合、旧Audio DeviceのFrame Timelineを新Deviceへそのまま継続しない方針とします。

基本手順:

1. 旧Audio Callbackを安全に停止
2. Pending Audio Commandを無効化 / Clear
3. Active Voiceを安全に停止または再初期化
4. 新DeviceのSample Rate / Buffer情報を取得
5. 新しいAudio Frame Originを確立
6. Scheduler Queueを再構築
7. Transport Stateとの対応を再確立

v0.1ではDevice Restartをまたいで鳴っていたVoiceをSample単位で完全継続することは必須にしません。

まずStuck Note、古いTimestampの誤実行、Crashを防ぐことを優先します。

## Sample Rate変更

ProjectのMusical TimeはSample Rateへ依存しません。

```text
960 PPQN Tick
→ 現在DeviceのSample Rateを使ってAudio Frameへ変換
```

そのため48 kHzから44.1 kHz等へDevice条件が変わっても、Sequencer Step位置自体は変化させません。

SamplerのResampling QualityやSynth DSP係数再計算はEngine / DSP仕様で別途定義します。

## Latencyに関するUI

v0.1ではUserへRaw Buffer Sizeを必ず表示・選択させる仕様にはしません。

まずBackendが推奨Low-latency設定を選択し、安定動作しない場合に安全なFallbackを取る方式を優先します。

将来Desktop版やAdvanced Settingsが必要になれば、以下のPreset方式を検討できます。

- Low Latency
- Balanced
- Stable

ただし初期UIへ技術Parameterを増やさない方針です。

## 計測 / Diagnostics

Prototypeでは最低限以下を計測可能にします。

- 実際のSample Rate
- 実際のCallback Buffer Frames
- Callback処理時間
- Callback Deadlineに対するCPU使用比率
- Underrun / Dropout Count
- Scheduler Queue High-water Mark
- Queue Overflow Count
- Active Voice Count / Peak

Debug / Development Buildで観測できればよく、v0.1 User UIへ常時表示する必要はありません。

## v0.1で確定するScope

- Audio Logicを特定Buffer Sizeへ固定しない
- 128 Frames程度をPreferred Targetとする
- 256 Frames程度を安定Fallbackとして許容する
- 512 Frames以上でもCompatibilityとして動作可能な構造を維持する
- 64 Frames以下は必須にしない
- Sequencer Lookaheadは初期約50 msをTarget候補とする
- LookaheadはEvent発音Latencyそのものではない
- Live InputはSequencer Lookaheadを待たないLow-latency Pathを持つ
- Schedulerは複数Buffer分のSafety Coverageを維持する
- QueueはBoundedとし十分なHeadroomを持つ
- Overflow時もAudio ThreadをBlockしない
- Stop / Panic系CommandをQueue飽和で失いにくいFail-safeを持つ
- Device Restart時はAudio Frame OriginとQueueを再構築する
- Project Musical TimeはSample Rate変更から独立させる
- Buffer / Latencyの最終値は実機計測後に確定する

## 次の検討

- Global Polyphony / Performance Budget
- Sampler / Synth / FXのCPU Budget
- Framework / Audio Backend比較用Performance Requirements
- Sample Streaming / CachingとAudio Threadの関係
- Resampling / Interpolation Quality
