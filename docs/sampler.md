# Sampler仕様

## 目的

Samplerは短いDrum Hitだけでなく、長い音楽SampleやField Recordingも扱えるようにします。編集は原則として非破壊です。

## 初期Parameter Set

```text
Sample
├─ file reference
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

## Waveform Editor

Sample EditorではWaveformを大きく表示し、Start / End MarkerをDragできるようにします。

Touch操作では以下を想定します。

- Start / End Markerの移動
- 将来的なWaveform Zoom
- Audition / Playback

Waveform描画ではPCM Sampleをすべて直接描かず、画面幅に合わせてPeakを縮約した表示データを使用します。

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

TrackはDefault Sampleを持ちますが、将来的にはStepごとに別Sampleを参照できるようにします。

```text
Track default: snare_01.wav
Step 9 override: clap_01.wav
```

## Slice — 将来機能

Sampleは複数のSliceを保持可能にします。

```text
Slice 1: start/end
Slice 2: start/end
...
```

将来のSlice作成方式:

- Equal Divisions
- Transient Detection
- Beat Grid

StepからSliceを直接選択できるようにします。

## Multisample — 将来機能

データModelは「Pitched Instrumentは必ず1 Sample」と固定しません。将来的にはKey / Velocity RangeごとにSource Sampleを切り替えるMultisample Mapへ拡張可能にします。

## Recording — 将来機能

想定Workflow:

```text
Microphone/Input
→ Record
→ Waveform
→ Trim
→ Assign to Sampler
→ Sequence
```

最初のImplementation Milestoneでは必須としません。

## Time Stretch — 将来機能

Time Stretchは単純なPitch / Rate Changeとは別機能です。DSP難度が高いため明確に後回しとします。

## File SafetyとLicensing

UserがImportしたSampleはUser Assetとして扱います。Factory Sampleは、アプリへの再配布を許可するLicenseが明確なものだけを使用し、Project内で記録します。
