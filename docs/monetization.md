# 収益化方針

この文書は、Original Sequencerを将来的に公開・販売する場合の収益化方針を記録します。

現段階ではTechnology Prototypeおよび製品仕様の検証を優先しており、価格や販売方法を確定するものではありません。将来の設計判断で収益モデルが技術構成へ影響する可能性があるため、暫定方針として残します。

## 基本方針

第一候補は、**基本無料（Free）+ Pro機能の買い切り**です。

無料版でもアプリの価値を十分に体験でき、可能であれば1曲を完成できる範囲を提供します。そのうえで、より高度な制作を行いたいユーザーがPro機能を買い切りで解放できる構成を優先します。

初期段階からサブスクリプションを中心にはしません。

## 想定する収益源

### Free

基本的なSequencer、Sampler / Synth、Project作成・保存など、アプリの中心的な制作体験を提供します。

無料版の具体的な制限は未決定です。Track数、Project数、高度な編集機能などを候補としますが、無料版でも制作体験そのものが成立することを重視します。

### Pro Unlock

一度の購入で高度な機能を解放する方式を第一候補とします。

候補:

- Track / Pattern等の上限拡張
- Advanced Sequencing機能
- 高度なSampler / Synth機能
- MIDI連携
- Audio Export / Render
- 高度なHarmony機能
- その他、制作効率を高める機能

具体的にどの機能をFree / Proへ分けるかは、製品のPlayable Versionが成立してから再評価します。

### 追加コンテンツ

必要に応じて、追加コンテンツの買い切り販売も検討します。

候補:

- Drum / Sample Pack
- Synth Preset Pack
- Genre / Style Pack
- 将来の追加Instrument / Sound Engine

Factory Contentと有料追加コンテンツは、配布・再販売条件を含めLicenseを明確に管理します。

### Subscription / Cloud

サブスクリプションは、継続的な運用コストまたは継続的な価値提供が発生するサービスを導入した場合に検討します。

候補:

- Cloud Project Storage
- Device間同期
- Online Sharing / Collaboration
- 継続的に追加される大規模Sound Library

Sequencerの基本機能を利用し続けるためだけのサブスクリプションにはしない方針を優先します。

## 広告

制作中の操作やAudio体験を阻害する可能性があるため、広告を主要な収益源にはしない方針です。

将来広告を検討する場合も、Sequencer操作や再生を妨げないことを優先します。

## Platform展開との関係

Androidを最初のBring-up Platformとして検証していますが、将来的なWeb / iOS / Desktop展開も考慮します。

複数Platformへ展開する場合は、可能であれば共通Account / Entitlementで購入状態やProjectを扱える構成を検討します。ただし、各App Storeの課金・外部決済・デジタルコンテンツ販売に関する規約へ従う必要があるため、実装時点の最新規約を確認します。

収益化の都合だけでAudio EngineやProject Formatを特定Storeへ強く依存させないようにします。

## 現時点の優先順位

想定する導入順序は次の通りです。

1. 無料で成立するCore Groovebox / Sequencerを完成させる
2. Pro買い切りで解放する価値のある高度機能を整理する
3. 必要に応じて追加Sound / Preset Packを導入する
4. Cloud等の継続サービスが必要になった場合のみSubscriptionを検討する

## 未決事項

- Free / Proの具体的な機能境界
- Pro価格
- 追加Packの価格と内容
- Free版のTrack / Project等の制限
- Platform間でのPurchase / Entitlementの扱い
- Account Systemを導入するか
- Cloud機能を導入するか
- Storeごとの販売・課金方式

これらは現段階では固定せず、Prototype、Playable Version、ユーザー検証、運用コストを踏まえて決定します。
