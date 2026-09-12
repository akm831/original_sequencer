# Project保存 / Persistence仕様

## 目的

Projectを安全に保存・復元し、将来のVersion UpdateやPlatform変更でも可能な限り互換性を維持します。

Persistence Layerは音楽的Behaviorを決める場所ではなく、Project ModelとAssetをSerialization / Restorationする責務を持ちます。

## 基本方針

Projectは次の2種類の情報から構成します。

1. 構造化されたProject Metadata
2. Audio SampleなどのBinary Asset

v0.1ではMetadataはJSON互換の明示的Schemaとして扱う方針です。

将来、保存UI上は1つのProject File / Bundleとして見せても、内部概念としてMetadataとAssetを分離します。

## Project Schema

概念例:

```text
ProjectFile
├─ schemaVersion
├─ appVersion
├─ project
│  ├─ id
│  ├─ name
│  ├─ bpm
│  ├─ ppqn
│  ├─ masterSwing
│  ├─ patterns[]
│  ├─ sampleLibrary[]
│  └─ future: arrangement
└─ assets[]
```

`schemaVersion`は必須です。

Application Versionとは別に保存形式Versionを持ちます。

例:

```text
schemaVersion = 1
appVersion = "0.1.0"
```

これによりApplication Versionが上がっても、Project Schemaが変わらない場合を区別できます。

## Serialization Format

v0.1のMetadataはJSON互換形式を第一候補とします。

理由:

- 人間が確認しやすい
- Debugしやすい
- Codex / Testから扱いやすい
- Platform非依存
- Schema Migrationを明示しやすい

JSON自体を永続的な外部公開Formatとして固定する必要はありません。

将来、PerformanceやFile Size上の理由が生じた場合でも、Domain ModelとPersistence Schemaを分離しておけば内部Formatを変更できます。

## Asset Reference

SampleをTrackやStepからFile Pathで直接参照しません。

概念:

```text
SampleAsset
├─ id
├─ fileName
├─ relativePath
├─ contentHash
├─ fileSize
├─ sampleRate
├─ channels
└─ optional: originalSourceInfo
```

Track / Sampler Stateは以下のようにAsset IDを参照します。

```text
sampleAssetId = "sample_01H..."
```

これによりProject Folderが移動しても内部参照が壊れにくくなります。

## Portable Project

Userが外部SampleをImportした場合、原則としてProject管理Assetへ取り込みます。

概念構造:

```text
MySong/
├─ project.json
└─ assets/
   ├─ kick.wav
   ├─ vocal.wav
   └─ field_recording.wav
```

DesktopではFolderまたは単一Bundle、MobileではApp Sandbox内Packageとして実装できます。

UI上の見え方と内部Storage方式はPlatformごとに異なって構いません。

重要なのはProject MetadataがPlatform固有の絶対Pathだけへ依存しないことです。

## Asset Import Policy

Sample Import時の基本方針:

1. Userが外部Audioを選択
2. Appが読込可能か検証
3. Project AssetへCopy / Import
4. Stable Asset IDを発行
5. SamplerはAsset IDを参照

元ファイル自体は変更しません。

Projectに取り込んだSampleに対するStart / End / Reverse / Root Noteなどは、Audio FileではなくProject Metadata側に保存します。

## Duplicate Asset

同じAudio Fileを何度もImportした場合、将来的にはContent Hashを使って重複を検出できます。

ただしv0.1で高度なDeduplicationは必須としません。

最初は正しい保存・復元を優先します。

## Factory Content

Factory SampleやFactory Presetについても、ProjectからStable IDで参照できる構造を検討します。

ただしApplication UpdateでFactory Contentが変更・削除されても古いProjectが壊れないことが重要です。

安全な方針候補:

- Project保存時に使用中Factory SampleをProject AssetへCopy
- Factory Assetに永続的なVersioned IDを付与

具体方式はFactory Content実装時に決定します。

## Synth Preset

Synth PresetはAudio FileではなくParameter Stateです。

概念:

```text
SynthPreset
├─ schemaVersion
├─ engineType
├─ engineVersion
├─ name
└─ parameters
```

Projectを保存する際は「Preset名だけ」を保存せず、その時点の実際のEngine StateをProjectへ保存します。

これにより、後でPreset Libraryが変更されてもProject Soundが変わりません。

Preset名やPreset IDは由来を表示するための補助Metadataとして保持できます。

Sampler Presetも同様に、再生Parameter StateとAsset Referenceを保存します。

## Schema Versioning

すべてのProjectに`schemaVersion`を持たせます。

Schema変更時はVersionごとのMigrationを用意します。

概念:

```text
v1 Project
   ↓ migrateV1ToV2
v2 Project
   ↓ migrateV2ToV3
v3 Project
```

古いVersionから最新Versionへ順番にMigrationします。

Migrationは可能な限り純粋なData Transformとして実装し、Audio EngineやUIへ依存させません。

## Forward Compatibility

新しいApplicationで古いProjectを開くことは明確にSupport対象とします。

逆に、古いApplicationで新しいSchema VersionのProjectを開けることは保証しません。

未対応の新しいSchemaを検出した場合は、破損扱いにせず「このProjectは新しいVersionで作成されています」と明示します。

## Save Safety

Project保存時に既存Fileへ直接上書きして途中失敗するとProjectが破損する可能性があります。

そのため、実装時にはAtomic Save相当の方式を優先します。

概念:

```text
project.tmp へ書込
↓
書込完了・Validation
↓
旧Projectと置換
```

PlatformごとのFile APIに合わせて具体実装します。

## Autosave

Autosaveは将来ではなく比較的早い段階で検討する価値がありますが、v0.1の最小PersistenceではManual Save / Loadを先に成立させても構いません。

将来候補:

- Edit後のDebounced Autosave
- Recovery Snapshot
- Last Known Good Save

Audio Recording中などTiming-criticalな処理と重いDisk I/Oを競合させないようにします。

## Missing Asset

Project Metadataが参照しているAudio Assetが見つからない場合でも、Project全体をOpen失敗にしません。

該当Sampleは`missing`状態として読み込みます。

例:

```text
SampleAsset
id: sample_123
status: missing
expectedFileName: vocal.wav
```

UIでは以下を可能にします。

- Missing Sampleを表示
- 他Track / Patternは通常利用
- Locate / Relink
- 必要ならReplace

Missing Sampleを自動的に同名の別Fileへ無条件で接続しないようにします。

将来Content HashやFile Metadataを使って安全な候補提示は可能です。

## Relink

Relink時:

1. Userが代替Fileを選択
2. Fileを検証
3. 該当Asset IDとの対応を更新
4. Sampler Parameterは維持

必要に応じてSample Length違いによるStart / End位置の再解釈Ruleを定義します。

## Project Validation

Load時には最低限以下を検証します。

- Schema Version
- 必須Field
- ID Reference整合性
- Numeric Range
- Asset存在
- Engine Type

未知のOptional Fieldは可能な限り無視できる設計を優先します。

破損した1 Track / AssetのためにProject全体を失わないよう、Recoverable ErrorとFatal Errorを分けます。

## Stable IDs

以下は表示名ではなくStable IDで参照します。

- Pattern
- Track
- Sample Asset
- Preset由来情報
- 将来Arrangement Item

名前変更によってReferenceが壊れないようにします。

ID形式はUUID等を候補としますが、具体LibraryはTechnology選定後に決定します。

## 保存しないRuntime State

原則として以下はProjectの音楽内容ではなくRuntime Stateです。

- 現在のAudio Buffer
- Scheduler Queue
- Playback中Voice
- UI Animation State
- Temporary Drag State

Current Pattern選択やEditor PageなどのUI StateをProjectへ保存するかは、User Experienceとして必要なものだけ選別します。

## v0.1 Persistence Scope

最初の実装目標:

- Explicit `schemaVersion`
- Project Metadata Save / Load
- Project Global Settings
- Patterns / Tracks / Steps
- Engine State
- Mixer State
- Sample Library Metadata
- Imported Sample Asset Reference
- Missing Asset検出
- Stable IDs
- Basic Validation

後回し可能:

- Single-file compressed Bundle
- Cloud Sync
- Cross-device Sync
- Advanced Deduplication
- Backup History UI
- Full Autosave / Recovery UI
- Project Archive Export

## 設計原則

- Projectは絶対Pathだけへ依存しない
- Imported SampleはPortable Project Assetとして扱う
- Preset名ではなく実際のSound Stateを保存する
- Schema Versionを必ず持つ
- Migrationを明示的に行う
- Missing Assetがあっても可能な限りProjectを開く
- Data ModelとFile Format実装を分離する
