# Sampler仕様

## 目的

Samplerは短いDrum Hitだけでなく、長い音楽SampleやField Recordingも扱えるようにします。編集は原則として非破壊です。

SamplerのAudio SourceはProject内のStable Asset IDで参照し、特定端末の絶対File Pathへ直接依存しない構造にします。

## 初期Parameter Set

```text
Sample
├─ sampleAssetId
├─ start
├─ end
├─ reverse
└─ root note

Playback
├─ One Shot
├─ Gate
└─ Loop

Pitch
├─ coarse tune
└─ fine tune

Amp
├─ attack
├─ decay
├─ sustain
└─ release

Filter
├─ type
├─ cutoff
└─ resonance
```

## Sample Asset Model

Sampler TrackはAudio File Pathではなく、Project Sample Library内のAsset IDを参照します。

概念:

```text
SampleAsset
├─ id
├─ displayName
├─ fileName
├─ relativePath
├─ contentHash
├─ fileSize
├─ format
├─ sampleRate
├─ channels
├─ frameCount
├─ duration
├─ bitDepth / encoding information
├─ waveformCacheRef
└─ optional originalSourceInfo
```

Sampler State:

```text
sampleAssetId = "sample_01H..."
```

Start / End、Reverse、Root Note、Loop設定等はAsset File自体へ書き込まず、Project / Sampler Metadataとして保持します。

## Import Workflow

初期Import Flow:

```text
User selects audio file
        ↓
Validate / decode probe
        ↓
Read basic audio metadata
        ↓
Copy into Project-managed assets
        ↓
Generate stable Sample Asset ID
        ↓
Generate waveform cache
        ↓
Assign Asset ID to Sampler
```

Import元のAudio Fileは変更しません。

Import処理が途中で失敗した場合は、不完全なAsset ReferenceをProjectの正常なSampleとして確定しないようにします。

大きなFileではUIをBlockしないよう、Copy / Decode / Waveform解析をTiming-criticalなAudio Threadから分離します。

## Supported Audio Format方針

Platform / Audio Backendが未決定のため、すべてのCodec対応を現時点で固定しません。

ただし製品仕様として、v0.1の**必須Import Format**は以下を基本候補とします。

- WAV / WAVE
- AIFF / AIF

理由:

- Uncompressed PCMを扱いやすい
- Sample編集・波形解析・Loop Point処理と相性がよい
- Desktop / Mobile間で共通実装しやすい

追加候補:

- FLAC
- MP3
- AAC / M4A

圧縮Formatは便利ですが、実際の対応可否はPlatform Codec、License、Decoder実装、Seek精度を確認して決定します。

v0.1で対応外Formatを選んだ場合は、無言で失敗せず「このFormatは現在Importできません」と明示します。

## Internal Audio Representation

Import元のSample RateやBit Depthをその場で破壊的に変更することは原則避けます。

Audio Engine内部では、再生時に必要なSample Rate変換やFloating-point処理を行える構造とします。

Project Assetとして元Fileを保持するか、Import時に内部標準Formatへ変換するかはAudio Backend選定時に最終決定します。

初期の安全な方針は、対応Formatの元FileをProject Assetとして保持し、非破壊にPlayback Parameterを適用することです。

## Mono / Stereo

v0.1ではMono / Stereo Sampleを扱えることを目標とします。

Sampler TrackはChannel数を意識せずTriggerでき、Mixer側では最終的にStereo Busへ送ります。

Multichannel Audio Fileへの対応は初期版では必須としません。

## Waveform Editor

Sample EditorではWaveformを大きく表示し、Start / End MarkerをDragできるようにします。

Touch操作では以下を想定します。

- Start / End Markerの移動
- 将来的なWaveform Zoom
- Audition / Playback

Waveform描画ではPCM Sampleをすべて直接描かず、画面幅に合わせてPeakを縮約した表示データを使用します。

## Waveform Cache

Waveform表示用DataはSource Audioの正本ではなく、再生成可能なCacheとして扱います。

基本方針:

- PCM全SampleをUI描画用に保持しない
- 区間ごとのMin / Max Peakを保存する
- Zoom Levelに応じて複数解像度を使える構造を許容する
- Cacheが消失・破損してもSource Audioから再生成できる
- Cache FormatはProject Schemaより柔軟に変更可能にする

概念:

```text
WaveformCache
├─ cacheVersion
├─ sourceAssetId
├─ sourceFingerprint / contentHash
├─ channels
└─ peakLevels[]
```

例えば高解像度Levelでは短いFrame WindowごとにPeakを持ち、低解像度Levelではより大きなWindowへ縮約します。

これにより短いDrum Sampleだけでなく、数十秒〜数分のField Recordingでも毎回全PCMをUIへ渡す必要がなくなります。

## Waveform Cacheの無効化

以下の場合はCacheを再生成します。

- Cache Versionが未対応
- Source AssetのContent Hash / Fingerprintが変化
- Cache FileがMissing / Corrupt

Start / End MarkerやReverseを変更しただけではSource Audioは変わらないため、通常Waveform Peak Cacheの再生成は不要です。

## Start / End Position

Start / Endは画面PixelやMillisecondsだけで保存しません。

内部候補はSource Audio上のFrame Positionまたは0.0〜1.0のNormalized Positionです。

精密編集、Slice、Relink時の挙動を考えると、Source Frame基準を有力候補とします。

例:

```text
startFrame = 1200
endFrame   = 48200
```

最終方式はAudio Decoder / Streaming Architectureと合わせて確定します。

## Audition

Sample Browser / Waveform EditorからSequencerを再生せずにAuditionできるようにします。

Auditionは通常のSequencer Triggerとは分離し、ProjectのStep Dataを書き換えません。

将来候補:

- Original Sample全体のPreview
- Start / End適用後のPreview
- Pitch / Filterを含むSampler StateでのPreview

v0.1では少なくとも選択Sampleをすぐ聴けることを優先します。

## Playback Mode

### One Shot

Triggerされたら指定Sample RangeのEndまで再生します。StepのNote Lengthでは停止しません。

Drums / Percussion向けです。

### Gate

Note / GateがActiveな間だけ再生し、Amp Envelopeにも従います。

Melodic Sample、Vocal Fragment等に向きます。

### Loop

VoiceがActiveな間、指定区間を繰り返します。

初期実装ではStart / EndをLoop Regionとして兼用し、専用Loop Start / End Markerは後で追加可能とします。

## Pitch

初期目標:

- Coarse: 少なくとも ±24 semitones
- Fine: ±100 cents

Root Noteが設定されている場合はKey Trackingを行います。

```text
root note = C3
C3 -> original pitch
E3 -> +4 semitones
G3 -> +7 semitones
```

これにより簡易Melodic Samplerとして利用でき、将来のChord Playbackにも接続できます。

## Reverse

Reverseは非破壊でPlayback Directionのみ変更し、Source Sample Fileは変更しません。

## Filter

初期Type:

- Low Pass
- High Pass

実装負荷が低ければBand Passも追加候補です。

可能であればSamplerとSynthでFilterの概念・実装を共有します。

## Parameter Lock連携

将来のP-Lock対象候補:

- Sample Start
- Sample End（音楽的に有用なら）
- Pitch Coarse / Fine
- Reverse
- Amp Envelope Parameters
- Filter Cutoff
- Filter Resonance

## Sample Override / Sample Lock

TrackはDefault Sampleを持ちますが、将来的にはStepごとに別Sample Asset IDを参照できるようにします。

```text
Track default: sample_snare_01
Step 9 override: sample_clap_01
```

表示上はFile Nameを見せても、内部ReferenceはStable Asset IDを使用します。

## Missing / Relink

Samplerが参照するAssetがMissingでもProject全体はOpen可能にします。

該当SamplerはMissing状態を表示し、発音しません。

UI候補:

- Locate
- Relink
- Replace

Relinkで長さの異なるFileを選択した場合、Start / End Frameが範囲外になる可能性があります。

安全な初期Ruleとして:

- 新しいFileを検証
- 既存Start / Endが有効なら維持
- 範囲外の場合はClampし、Userへ状態を通知

を候補とします。

Content Hashが一致するFileを選択した場合は、同一Asset候補として高い信頼度でRelinkできます。

## Memory / Streaming方針

短いDrum Sampleと長いRecordingでは最適なMemory戦略が異なります。

将来のAudio Backendでは以下を分離できる構造にします。

- Short Sample: MemoryへPreload
- Long Sample: Streaming / Chunked Reading候補

ただしv0.1では先に機能を成立させ、実機Memory使用量を計測してThresholdを決めます。

「すべてのSampleを常に全PCM Memoryへ展開する」ことを永続的な前提にはしません。

## Slice — 将来機能

Sampleは複数のSliceを保持可能にします。

```text
Slice 1: start/end
Slice 2: start/end
...
```

Slice PositionもSource Audio上の安定したPosition表現を使用します。

将来のSlice作成方式:

- Equal Divisions
- Transient Detection
- Beat Grid

StepからSliceを直接選択できるようにします。

Waveform Cache / Peak Dataは将来のTransient Detection用解析Dataとは分離します。表示Cacheを音楽解析の正本にしません。

## Multisample — 将来機能

データModelは「Pitched Instrumentは必ず1 Sample」と固定しません。将来的にはKey / Velocity RangeごとにSource Sampleを切り替えるMultisample Mapへ拡張可能にします。

## Recording — 将来機能

想定Workflow:

```text
Microphone/Input
→ Record
→ Project Assetとして確定
→ Waveform Cache生成
→ Waveform
→ Trim
→ Assign to Sampler
→ Sequence
```

録音したAudioもImport Sampleと同じSample Asset Modelへ入れることで、その後のSampler / Slice / Persistenceを共通化します。

最初のImplementation Milestoneでは必須としません。

## Time Stretch — 将来機能

Time Stretchは単純なPitch / Rate Changeとは別機能です。DSP難度が高いため明確に後回しとします。

## File SafetyとLicensing

UserがImportしたSampleはUser Assetとして扱います。Factory Sampleは、アプリへの再配布を許可するLicenseが明確なものだけを使用し、Project内で記録します。

User Assetの元FileをImportや非破壊編集によって変更しません。

## v0.1 Asset / Import Scope

初期目標:

- Project Asset IDによるSample参照
- External Audio Import
- WAV / AIFFを最低限の対象候補とする
- Mono / Stereo
- Basic metadata read
- Project-managed asset copy
- Waveform peak cache
- Start / End waveform editing
- Audition
- Missing Asset表示
- Relink / Replaceへの拡張可能性

後回し可能:

- MP3 / AAC / FLACの完全Cross-platform対応
- Advanced background analysis
- Transient detection
- Automatic BPM / Key analysis
- Long-file streaming optimization
- Advanced deduplication
- Destructive audio conversion
