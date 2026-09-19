# VFX制作方針

## 目的

本資料は、Wave Survivalのプレイ中に必要なVFX、その実装可能範囲、必要なテクスチャ、制作順を定める。最初の目標は、攻撃の手応え、敵撃破、時間回復、Wave進行を一目で区別できることとする。

ゲーム進行とHUDの仕様は[GAME_LOOP.md](GAME_LOOP.md)、現行描画基盤は[RENDERING.md](RENDERING.md)を正とする。

## 参照したVFXコンテキスト

共通リポジトリの`knowledge/vfx-reference-contexts/`から、次の記録を参照した。

| 記録 | 今回に活かす抽象化した特徴 | 採用候補 |
| --- | --- | --- |
| `2026-09-13-fab-preview-observations.md` | 高輝度コア、同心リング、短い放射線、少数の粒子を重ねてイベントの核を作る | 攻撃命中、敵撃破、Wave開始 |
| 同上: Immortal Loot Drop VFX | 地面のリング、縦ビーム、上昇粒子でドロップ位置を示し、色で価値を区別する | 時間回復アイテムの待機表示 |
| 同上: Sparks Embers | 小発光から細い火花を散らし、速度・寿命・色で用途を分ける | 命中・撃破の火花 |
| 同上: Light Vortex Shader | 円形マスクと流れる発光帯を分離する | Wave開始／敵出現の地面リング（後段） |
| `2026-09-13-unreal-vfx-asset-collection.md` | SpriteとRibbonを短いフィードバックとして組み合わせる | アイテム取得、ダッシュの軌跡 |

上記は静止プレビューおよびアセット構造からの観察であり、固有の意匠・パラメータを複製しない。時間変化やBlend設定が未確認の内容は、本ゲーム用の実装仮説として扱う。

## 現行エンジンで使える表現

| 手段 | 現時点で可能なこと | 制約・注意 |
| --- | --- | --- |
| `ParticleEmitter` + `ParticleRenderer` | 点／円／球からのバースト・ループ、方向・コーン・球状の初速、重力・抵抗、寿命ごとのサイズ／色カーブ、カメラ正対Quad | 粒子ごとの回転、UVアニメーション、フリップブック、衝突、吸引、ノイズ移動、ローカル空間シミュレーションはない。基本は加算マテリアルで使う。 |
| `TrailRenderer` | 追従リボン、カメラ正対または固定方向、寿命、幅・色カーブ、距離による頂点間引き | UVはリボン全長へ0–1で割り当てる。スクロール、ノイズ変形、断続的な帯はShader追加が必要。 |
| Quad + `ModelRenderer` | ビルボード、透明／加算マテリアル、Material Property Overrideによる色・UV・強度の個別制御 | 生存時間、拡縮、回転、UVスクロールはScript側で制御する。 |
| Material / Shader | `UnlitAdditive`、`UnlitTransparent`、独自Shader、テクスチャ、色、UV Scale／Offset | 歪み、Depth Fade、Soft Particle、フリップブックは既製Shaderにない。 |
| PostProcess | Bloomの抽出・ぼかし・合成。画面全体Shaderの追加基盤もある | 現在登録済みなのはBloomのみ。FXAA、色収差、Vignette、画面ノイズは未実装。 |
| UI Shader | 文字のOutline、UIのマスク・グラデーション、既存Bloomとの組合せ | ワールドVFXとは分離し、時間警告や回復数値の視認性を補助する。 |

透明物はRenderer単位で奥から手前へ並ぶ。初期VFXは加算合成を主にし、半透明の大きな煙を重ね過ぎない。これによりソート破綻とオーバードローを避ける。

## 必要なVFX

### 初回実装の必須セット

| 優先 | 契機 | プレイヤーに伝えること | レイヤー | 実装候補 |
| --- | --- | --- | --- | --- |
| P0 | 攻撃準備 | 引っ張り中で、発射方向を調整している | 既存のShotDirector、中心の弱いパルス | ShotDirector Shader、必要ならQuad |
| P0 | 攻撃開始 | プレイヤーがダッシュへ移った | 足元リング、前方への短い閃光、移動中の細いTrail | Quad、`TrailRenderer` |
| P0 | 攻撃命中 | 敵へ有効打を与えた | 白いヒットフラッシュ、シアンの発光核、放射火花 | 既存の白フラッシュとObjectShake、`ParticleEmitter` |
| P0 | 敵撃破 | 撃破が確定し、ドロップが出た | 外向きリング、火花、短い残光 | Quad、`ParticleEmitter`、既存Explosionを必要に応じて流用 |
| P0 | 時間回復アイテム出現・待機 | 取得対象の位置と時間回復の価値 | 小さな地面リング、控えめな縦ビーム、上昇粒子 | Quad、ループ`ParticleEmitter` |
| P0 | 時間回復アイテム取得 | 時間が回復した | アイテム位置のフラッシュ、プレイヤーへ向かう短いTrail、`+秒` UIポップ | Quad、`TrailRenderer`、GameHud通知 |
| P0 | Wave開始／クリア | 戦闘開始、または時間回復を伴う一区切り | `WAVE n`／`CLEAR +秒` UIポップ、HUDラインの発光パルス | GameHud、UIText／UIImageのMaterial Override |
| P0 | 残り時間警告 | 直ちに敵撃破・取得が必要 | TIME文字とゲージの短い赤橙パルス | UI Shader、Bloom調整 |

### 後段へ回す表現

| 優先 | 契機 | 理由 |
| --- | --- | --- |
| P1 | 敵出現の地面リングと上昇演出 | 遠方出現の発見性を上げられるが、戦闘の核より優先度を下げる。 |
| P1 | MovingEnemy専用の残像／移動Trail | 敵種の識別に有効。通常敵との色・密度の差を設計してから追加する。 |
| P1 | HeavyEnemy専用の重量感ある接地波・低速粒子 | Heavyの行動差が確定してから作る。 |
| P1 | TimeUpの画面色変化またはVignette | まずUI警告で十分な視認性を確認する。 |
| P2 | ノイズ歪み、深度フェード煙、フリップブック炎、画面空間の屈折 | 現行機能では未対応で、専用ShaderまたはParticleSystem拡張が必要。 |

## VFX Prefabと通知の方針

イベントごとに一度だけ生成する効果は、Prefabとして分ける。名前は`VFX_`接頭辞を使い、ロジックを持つ側は見た目を直接組み立てず、Prefab生成と必要最小限の色・強度だけを渡す。

| Prefab候補 | 発生元 | 寿命 | 初期構成 |
| --- | --- | --- | --- |
| `VFX_PlayerLaunch` | `Player`がAttackingへ遷移 | 0.15–0.25秒 | 発光リング、前方フラッシュ |
| `VFX_Collision` | `PlayerHit`で有効打 | 0.80秒以内 | 接触法線の両側へ広がる橙・黄橙・赤橙のコーン火花 |
| `VFX_EnemyDeath` | `Enemy::FinalizeDeath` | 0.25–0.45秒 | 外向きリング、球状火花 |
| `VFX_TimeItemIdle` | `TimeRecoveryItem::OnStart` | アイテム消滅まで | 縦ビーム、低密度上昇粒子 |
| `VFX_TimeItemPickup` | `TimeRecoveryItem::OnHitBegin` | 0.20–0.35秒 | フラッシュ、短いTrail |
| `VFX_WaveStart` / `VFX_WaveClear` | `WaveDirector` | 0.30–0.60秒 | 原則HUD内の発光パルス |

初回は各Prefabを一回生成して破棄する。複数のEmitterを親でまとめる短命Prefabには、ParticleEmitterの`finishedDestroy`だけで寿命を表せない場合がある。そのときだけ、親を指定時間で破棄する共通の`VfxLifetime`を追加する。プール、吸引軌道、属性別の色テーブルはP0の手動確認後に判断する。

## テクスチャ方針

色はテクスチャへ焼き込まず、白または明度情報を主にしたRGBA PNGを用途別Materialで着色する。火炎本体はAlphaBlend、発光核はAdditive／Emissionを使い分ける。素材は`Assets/Local/Textures/VFX/`を配置先候補とし、生成・配置は別途指示を受けてから行う。

### 既存素材

| 素材 | 確認済みの用途 | 再利用判断 |
| --- | --- | --- |
| `Assets/Local/Textures/particle.png` | `Particle.material`の既定テクスチャ | P0の火花・上昇粒子をまずこれで検証する。 |
| `Assets/Local/Textures/Explosion.png` | 4×4の既存Explosionスプライトシート | 敵撃破で試用可能。ただし色・形がゲームのシアン基調に合わなければ置き換える。 |
| `Assets/Local/Textures/trail.png` | VFX用Trail候補として存在 | ダッシュ／取得Trailの試作で確認する。 |
| `Assets/Local/Textures/Ripple.png` | リング／波紋候補として存在 | 足元リング・撃破波紋の試作で確認する。 |
| `Assets/Local/Textures/Line.png` | 線状素材として存在 | 放射線または細いTrailの試作で確認する。 |

既存素材の見た目・解像度・アルファチャンネルは、Prefabへ採用する前にGame Viewで確認する。ファイル名だけからタイル性や透明度を断定しない。

### 追加が必要な候補

| ファイル候補 | 目安 | アルファ | 役割 | 使用先 |
| --- | --- | --- | --- | --- |
| `vfx_soft_circle.png` | 256×256 | 外周が滑らかに消える | 発光核、フラッシュ、粒子 | 命中、取得、Wave通知 |
| `vfx_ring.png` | 512×512 | 内外縁を柔らかく透過 | 同心リング、接地波 | 発射、撃破、アイテム出現 |
| `vfx_spark.png` | 256×256 | 細い菱形または短線だけを不透明にする | 火花、放射線 | 命中、撃破 |
| `vfx_trail.png` | 512×64 | 横方向に連続し、端をフェード | 発光リボン | ダッシュ、取得 |
| `vfx_noise.png` | 256×256 | グレースケール | 後段のUV歪み、リングの揺らぎ、ディゾルブ | P1以降 |

`vfx_soft_circle`、`vfx_ring`、`vfx_spark`、`vfx_trail`の4枚でP0を完結させる。フリップブック素材は、ParticleSystem側に対応機能を追加するまで作らない。

## Shaderとポストプロセスの方針

1. P0は用途に応じてAlphaBlend／Additiveを使い分け、発光はParticle側の`EmissionColor`と既存Bloomで制御する。色・Alpha・UV Scale／OffsetはMaterial Property Overrideで制御する。
2. リングの拡縮、フラッシュのフェード、Trailの色変化はPrefabのScriptと既存Curveで成立させる。最初から汎用の複雑なVFX Shaderを作らない。
3. 現行Bloomは常時の弱い発光を保ち、命中・取得・警告だけが明確に明るくなるよう、ThresholdとIntensityを先に調整する。
4. TIME警告はまずUIの赤橙パルスで実装する。画面全体のVignette／色調変化は、UIだけで気付けないことが確認できた場合に1 PassのPostProcessとして追加する。
5. FXAAはUI斜線の改善候補ではあるが、ワールドVFX制作の前提にはしない。ポスプロ整備時に、Bloomとの順序と解像度低下を並べて評価する。

## Engine側設計メモ（次回実装セッション）

### MeshとRendererの役割

新しいMeshアセットを増やす必要はない。既存のPrimitive Quadを共通形状として使い、描画責務を分ける。

| 用途 | 構成 |
| --- | --- |
| 衝突コア、リング、フラッシュ | `ModelRenderer` + Primitive Quad |
| 大量の火花・炎粒子 | `ParticleEmitter` + `ParticleRenderer` |
| 連続した軌跡 | `TrailRenderer` |
| 立体的な破片・特殊形状 | `ModelRenderer` + Mesh |

`ModelRenderer`は単体Meshの描画用であり、粒子の寿命・色・サイズ・ランダム性・バッチ描画を代用しない。`ParticleRenderer`は既存Quadを内部で再利用するため、粒子表現のために個別GameObjectや新規Meshを作らない。

### Particle基盤で実装する項目

1. 粒子データへ`size`、`rotation`、`emissionColor`、`age`、`lifetime`、`randomSeed`を持たせる。
2. Emitterへ`sizeMin／Max`、`rotationMin／Max`、`angularVelocity`、`emissionColorOverLifetime`、`emissionStrength`を追加する。
3. `ParticleRenderer`を粒子単位のDrawからInstancing描画へ変更する。Instancing用Shaderは`[InstancedStruct] struct ObjectInput`を宣言し、リフレクションで構造体を認識してStructuredBufferへ配置する。各Instanceは`SV_InstanceID`で読み出し、ワールド行列はEngineが自動で埋め込む。
4. Particle Shaderを用途別に分け、火炎本体のAlphaBlendと発光核のAdditive／Emissionを同じ粒子データから選択できるようにする。

### Particle Instancing最小仕様

Instancingは汎用Rendererへ一度に広げず、まず`ParticleRenderer`だけを対象にする。Shaderに次の宣言がある場合だけInstancing用Shaderとして扱う。

```hlsl
[InstancedStruct]
struct ObjectInput
{
    float4x4 world;
    float4 color;
    float4 emissionColor;
};
```

Shaderリフレクションで`ObjectInput`のメンバーと型を取得し、EngineがStructuredBufferを生成・更新してShaderの入力スロットへバインドする。描画時は`SV_InstanceID`から該当Instanceを参照し、Instance数を`DrawIndexedInstanced`へ渡す。`world`行列はEngineが各粒子の位置・回転・サイズから自動生成するため、Game側ShaderやPrefabへ個別のワールド行列を記述しない。

通常のShaderは従来の描画経路を維持する。`ModelRenderer`や`TrailRenderer`までInstancing対応を拡張するのは、Particleでレイアウト・SRVスロット・寿命管理を検証した後に判断する。

### カメラ正対と方向表現

Quad全体を速度ベクトルへ向けるとカメラ正対が崩れて見えなくなるため、方向表現は「カメラ正対 + 画面内Z回転」とする。

1. ワールド速度をカメラのRight／Upへ射影し、画面上の2D方向を求める。
2. その2D方向からロール角（Z回転）だけを計算する。
3. ビルボード回転の後段へロールを合成し、カメラの正対姿勢は維持する。
4. 画面上の速度がほぼゼロの場合は、現在のCameraFacing姿勢へフォールバックする。

`BillboardType::VelocityAligned`のように3D姿勢へ向けるモードとは分け、`CameraFacingRoll`または同等の専用行列関数として実装する。炎や火花の長軸はこのロールと`velocityStretch`で表現する。

### 実装順

```text
Particleデータ／Curve拡張
→ CameraFacingRoll + 速度によるStretch
→ InstancedStruct／StructuredBufferによるParticle Instancing
→ Particle Shader（AlphaBlend／Additive）
→ VfxLifetime／VfxSequence
→ 各VFX Prefabの再構築
```

## 制作順

### Phase 0: 素材と基準の固定

1. 既存の`particle.png`、`trail.png`、`Ripple.png`、`Explosion.png`をGame Viewで確認する。
2. 足りない4種類だけを白基調の透明PNGとして用意する。
3. VFX用Materialは用途ごとにBlendを選ぶ。火花はAlphaBlendを基本とし、発光はParticle側のEmissionColorを追加してから調整する。

### Phase 1: 戦闘の核

1. `VFX_Collision`を実装し、白フラッシュ・HitStop・ObjectShakeと同フレームで発生させる。
2. `VFX_EnemyDeath`を実装し、敵の破棄がHitStop解除後である現在の仕様に合わせる。
3. `VFX_PlayerLaunch`と短いダッシュTrailを追加し、攻撃の開始と方向を明確にする。
4. 連続命中時に画面を覆わないか、敵とプレイヤーの位置関係が隠れないかを確認する。

### Phase 2: 時間を主役にする

1. `VFX_TimeItemIdle`を追加して、回復アイテムを敵本体・背景から区別する。
2. `VFX_TimeItemPickup`、TIMEの`+秒`ポップ、HUDの短い発光を同じ取得イベントへ接続する。
3. Waveクリア時は、PlayerがIdleになってから回復する既存進行に合わせて`CLEAR +秒`を出す。
4. 残り時間警告をUIパルスで追加し、見落としがないかを確認する。

### Phase 3: 演出の拡張と必要なエンジン対応

1. 敵出現、MovingEnemy、HeavyEnemyを専用色・形で差別化する。
2. 必要性が確認された機能だけをエンジン側へ追加する。優先候補は、カメラ正対を維持したParticleの画面内Z回転、速度Stretch、EmissionColor、UVスクロール、フリップブック、親を含むVFX寿命管理、軽量Vignetteである。
3. 追加したエンジン機能は`engine/*`ブランチで実装・検証し、ゲーム側へ取り込む。

## 受け入れ確認

- 攻撃開始、命中、撃破、ドロップ、取得、Wave開始／クリア、低時間警告を画面を見ただけで区別できる。
- 敵撃破のVFXはHitStop中に発生しても、アイテム取得より先に読める。
- 回復アイテムは遠距離でも位置を把握でき、取得時はTIME数値の増加と視覚的に同期する。
- 同時に複数の敵を倒しても、プレイヤー・敵・回復アイテムの位置が見えなくならない。
- 透明物のソート破綻、過剰なBloom、長く残る粒子がない。
- DebugビルドでVFXのPrefab生成・破棄を通し、複数Waveでメモリ増加やログエラーがない。

## 実装済み: 衝突火花

`PlayerHit`の有効打時に`HitInfo.point`へ`VFX_Collision`を生成する。`HitInfo.normal`はB（敵）からA（プレイヤー攻撃判定）を押し戻す方向なので、プレイヤー側Emitterは法線方向、敵側Emitterは反対方向へCone初速を設定する。

各側に橙・黄橙・赤橙のEmitterを1つずつ置き、機械生成した白い`VFX_Flame.png`をAlphaBlendで着色する。`CurveColor`は短い白寄りのピークから属性色へ遷移し、AlphaとScaleを寿命0–1でフェードさせる。複数Emitterの子Objectは親の`CollisionVfx`が寿命後にまとめて破棄する。現在のサイズ基準は`m_particleSize = [5,5,5]`で、炎感は色の温度差、上向きの重力、先端へ向けたScale減衰で作る。
