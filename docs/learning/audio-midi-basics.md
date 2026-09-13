# 音声・MIDI基礎ノート

## この文書の目的

このプロジェクトを設計・実装しながら必要になるAudio / MIDI / Sequencerの基礎知識を、プログラミングやDSPの専門知識がなくても読み返せる形で蓄積します。

仕様書は「このアプリをどう作るか」を定義しますが、このLearning Noteは「なぜその設計になるのか」を理解するための教材です。

---

## 1. PCMとは

PCM（Pulse Code Modulation）は、音の波形を時間ごとの数値として表現する代表的なデジタル音声方式です。

概念的には次のような大量の値になります。

```text
0.00
0.03
0.12
0.48
0.81
0.53
0.10
-0.31
-0.72
...
```

これらの値が時間順に並ぶことで、音の波形を表します。

大まかには、

```text
PCM = 実際の音を数値化したもの
```

と理解して構いません。

### Sample Rate

44.1 kHzなら、1秒間を44,100回のSample Pointで表します。

```text
44.1 kHz
= 44,100 samples / second
```

そのため1分では約264万Sample Pointになります（1 channelあたり）。

### WAVとの違い

PCMとWAVは同じ意味ではありません。

- PCM: 音を数値で表す方式
- WAV: 音声データなどを格納するFile Container / Format

WAVにはPCM Audioを格納できます。

このGrooveboxでは、Samplerが読み込んだWAV等をDecodeし、最終的にはAudio Engineが扱えるPCM相当のAudio Dataとして再生することになります。

---

## 2. MIDIとは

MIDIは基本的に「音そのもの」ではなく、「どう演奏するか」を伝える情報です。

例えば、

```text
Note On
Note = C4
Velocity = 100
```

は、

```text
C4をVelocity 100で鳴らす
```

という演奏命令に相当します。

その後、

```text
Note Off
Note = C4
```

を送れば、そのNoteを離すことを表します。

MIDIにはNote以外にも、例えば次のような情報があります。

- Note On / Note Off
- Note Number
- Velocity
- Pitch Bend
- Control Change
- Program Change
- Aftertouch
- Timingに関係する情報

大まかには、

```text
MIDI = 楽譜 + 演奏指示
PCM  = 実際の音
```

と考えると理解しやすくなります。

### MIDIだけでは音は決まらない

同じC4 / Velocity 100でも、受け取るInstrumentによって音は変わります。

```text
C4 / Velocity 100
       │
       ├─ Piano → ピアノ音
       ├─ Bass Synth → ベース音
       └─ Sampler → 読み込んだSample
```

MIDI Event自体にピアノやベースの録音が入っているわけではありません。

---

## 3. このGrooveboxでの関係

このプロジェクトではSequencerとSound Engineを分離します。

```text
Sequencer

Step
├─ notes[]
├─ velocity
├─ lengthTicks
└─ accent
       │
       ▼
Musical Event
       │
       ├──────────────┐
       ▼              ▼
    Sampler          Synth
       │              │
       ▼              ▼
 Sample Audio     Oscillator / DSP
       │              │
       └──────┬───────┘
              ▼
          Audio Output
```

Sequencerが扱うNote / Velocity / Lengthなどは、考え方としてMIDIに近い領域です。

SamplerはSample Audioを再生します。

SynthはOscillatorなどの計算からAudioを生成します。

最終的にはどちらもAudio Outputへ送られます。

この分離により、同じSequencer EventをSampler、Synth、将来のExternal MIDIへ利用できます。

---

## 4. Waveform表示とMin / Max Peak Cache

長いPCM Audioには非常に多くのSample Pointがあります。

例えば44.1 kHzの5分Audioなら、1 channelだけでも約1,323万Sample Pointあります。

しかしスマートフォンやPCのWaveform表示領域は、横幅が数百〜数千Pixel程度です。

そのため全PCM Sampleを毎回そのまま描画する必要はありません。

### Min / Max Peak

PCMを一定区間ごとにまとめ、その区間で最も大きい値と最も小さい値だけをWaveform表示用に保存します。

```text
PCM samples 1〜100
→ min = -0.64
→ max = +0.81

PCM samples 101〜200
→ min = -0.32
→ max = +0.56
```

Waveform UIはこのMin / Maxを使って各区間の上下方向の振幅を描画します。

これをPeak Cacheとして保存しておけば、Waveformを表示するたびに巨大なPCM全体を走査する必要がありません。

### Peak Cacheは音ではない

重要なのは、Peak Cacheは再生Audioではなく表示用の補助データだということです。

```text
Source Audio / PCM
       │
       ├─→ Sampler Engine → 音を鳴らす
       │
       └─→ Peak Cache → Waveformを描く
```

Peak Cacheを削除しても音質は変わりません。

元Audioから再生成できます。

### Zoom用の複数Resolution

将来的には複数段階のPeak Cacheを用意できます。

```text
全体表示   → coarse peaks
少しZoom  → medium peaks
大きくZoom → fine peaks
最大Zoom  → 必要ならPCMに近い表示
```

地図ApplicationがZoom Levelに応じて表示するDetailを変えるのと似た考え方です。

この方式なら、短いKickから数分のField Recordingまで同じWaveform Editorで効率よく扱いやすくなります。

---

## 5. よく混同しやすい用語

```text
PCM
音の波形そのものを数値化したAudio Data

WAV / AIFF
Audio Dataを保存するFile Format / Container

MIDI
NoteやVelocityなどの演奏情報

Sample
Samplerで再生するAudio素材を指すことが多い
ただし「PCMの1個の測定点」もsampleと呼ぶため、文脈に注意

Sample Rate
1秒間にAudio波形を何回測定するか
例: 44.1 kHz = 44,100回/秒

Peak Cache
Waveformを高速表示するためにPCMを要約した派生データ
```

特に`sample`という言葉は、

1. Kick.wavのような「音素材」
2. PCM波形を構成する1つのSample Point

の両方に使われるため注意します。このプロジェクトの仕様書では、必要に応じて`Sample Asset`と`PCM Sample / Sample Point`のように書き分けます。

---

## 6. 今後追加する教材候補

このLearning Note群には、プロジェクトを進めながら次の内容を追加していきます。

- Bit Depth / dB / Clipping
- Mono / Stereo / Channel
- Sample Rate Conversion
- Buffer Size / Audio Latency
- Audio Thread / Real-time Audio
- Oscillator / Frequency / Pitch
- ADSR Envelope
- Filter / Cutoff / Resonance
- LFO
- Polyphony / Voice
- MIDI Note Number
- PPQN / BPM / Musical Tick
- Swing / Micro Timing
- DSPの基本
- Audio File Decode / Encode

必要になった時点で、実際のGroovebox設計との関係を含めて追記します。
