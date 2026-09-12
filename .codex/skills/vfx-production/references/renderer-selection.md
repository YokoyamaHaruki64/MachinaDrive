# 現行VFX手段の選択

Project_Driveの現行実装を正とする。VFXを作る前に、イベントの主役になる形を一つ決め、最小の手段を選ぶ。

| 主役の形・変化 | 第一候補 | 補助候補 | 別手段にする条件 |
| --- | --- | --- | --- |
| 一瞬の命中、爆ぜる火花、短い放射 | `ParticleEmitter`のBurst | Quadの発光核 | 固定した絵柄や輪郭が主役 |
| 移動の軌跡、吸引経路、斬撃の線 | `TrailRenderer` | 先端のQuad／Particle | 移動履歴が不要 |
| リング、光柱、フラッシュ、魔法陣 | Quad + `ModelRenderer` | Particleで周辺粒子 | 粒子ごとの異なる運動が主役 |
| 周囲の気配、上昇粒子、低密度の常時演出 | Loopの`ParticleEmitter` | Quadのビーム | 正確な曲線軌道や物体追従が必要 |
| 回復値、Wave通知、低時間警告 | GameHudのUIText／UIImage | Bloom、短いワールドVFX | ワールド上の位置を示す必要がある |
| 画面全体の危機感 | UIの色・発光 | PostProcess | UIだけで十分に認識できる |

## ParticleEmitter / ParticleRenderer

加算スプライトを多数、短時間で出す手段。Particleの色はMaterialの`_BaseColor`へ掛かるため、白基調のTextureとCurveで色を作る。

| 設定 | 意味 | 選び方 |
| --- | --- | --- |
| `offset` | OwnerのWorld位置からの生成オフセット | 接地点、武器先端、モデル中心から調整する。 |
| `maxParticles` | 保持できる最大粒子数 | Burst数、または`emitRate × 最大寿命`を満たす最小値にする。 |
| `burst` | 一回放出か継続放出か | 命中・撃破はtrue、オーラ・上昇粒子はfalse。 |
| `burstCount` / `burstTime` | Burst総数と放出区間 | ほぼ同時なら衝撃、少し時間を持たせると破裂になる。 |
| `finishedDestroy` | Burst後に全粒子が消えたらOwnerを破棄 | 単一Emitterの短命Prefabにのみ使う。 |
| `emitRate` | ループ時の毎秒生成数 | 常時演出は低密度から始める。 |
| `lifetimeMin` / `lifetimeMax` | 粒子寿命の範囲 | 少し差を付けて規則性を崩す。フラッシュは短くそろえる。 |
| `gravity` / `drag` | 重力加速度と減衰 | 火花は重力、エネルギーは重力0・高めのdragを基準にする。 |
| `size` | Quadの基準サイズ | Textureの余白を含むため、Game Viewで実寸を決める。 |
| `scaleOverLifetime` | 正規化寿命0–1でのサイズ倍率 | 衝撃波は小→大、火花は大→小、核は急拡大後に消す。 |
| `colorOverLifetime` | 正規化寿命0–1でのRGBA | Alphaを確実に0へ落とし、色のピークと余韻を作る。 |
| `randomizeSpeed` | 初速の固定／ランダム | 輪郭の揃う放射は固定、自然な火花はランダム。 |
| `speed` / `speedMin` / `speedMax` | 初速 | 寿命と合わせて飛距離を決める。 |
| `velocityType` | `None`、`Directional`、`Cone`、`SphereVel` | 浮遊はNone、上昇はDirectional、前方火花はCone、爆発はSphereVel。 |
| `direction` | Directional/Coneの基準方向 | Ownerの回転を自動追従しない。必要ならScriptで更新する。 |
| `coneAngle` | Coneの広がり（度） | 小さいほど鋭く、大きいほど破裂感が出る。 |
| `spawnType` | `Point`、`Circle`、`Sphere` | 接触点はPoint、接地リングはCircle、爆発の厚みはSphere。 |
| `spawnRadius` | Circle/Sphereの生成半径 | 初期面積を決める。速度と同時に大きくし過ぎない。 |
| `spawnNormal` | Circleの法線 | 地面リングはUp。斜面追従はScript側で渡す。 |

### CurveとColorCurve

`Curve<float>`と`CurveColor`のキー時刻は、粒子寿命を0–1へ正規化した値である。キー間はBezier制御点で補間される。`CurveColor`はR/G/B/Aをそれぞれ独立したCurveとして持つ。

| 用途 | `scaleOverLifetime`の出発点 | `colorOverLifetime`の出発点 |
| --- | --- | --- |
| 発光核 | 0.2 → 1.2 → 0 | 白・高Alpha → 属性色・低Alpha → Alpha 0 |
| 放射火花 | 1.0 → 0.2 | 白／属性色 → Alpha 0 |
| 上昇粒子 | 0.4 → 0.8 → 0.3 | 属性色・中Alpha → 白寄り → Alpha 0 |
| 残滓 | 0.8 → 1.4 | 低彩度・低Alpha → Alpha 0 |

Alphaを単純に線形で落とすのではなく、核はピーク直後に急減衰、残滓は終盤まで薄く残すなど、イベントの時間構造に合わせる。現行Particleには粒子回転、UVアニメーション、フリップブック、衝突、吸引、ノイズ移動、ローカル空間シミュレーションはない。

## TrailRenderer

OwnerのWorld位置履歴を帯として描く。頂点年齢に応じて幅・色Curveを評価し、UVのV座標はリボン全長へ0–1で割り当てる。

| 設定 | 意味 | 選び方 |
| --- | --- | --- |
| `m_materialPath` | Trail専用の単一Material | 発光帯は加算Materialを第一候補にする。 |
| `m_widthCurve` | 頂点年齢0–1の幅倍率 | 根元を太く、古い端を細くして方向を読ませる。 |
| `m_colorCurve` | 頂点年齢0–1のRGBA | 新しい先端を明るく、古い部分は透明へ落とす。 |
| `m_offset` | Owner位置からの採取点 | 中心ではなく少し後方・足元・武器先端へずらす。 |
| `m_fixedDirection` | FixedDirection時の帯の向き基準 | 地面へ寝かせる帯に使う。 |
| `faceMode` | `CameraFacing`または`FixedDirection` | 空中の速度線はCameraFacing、接地線はFixedDirection。 |
| `m_trailLifetime` | 頂点を残す秒数 | ダッシュは短く、誘導線は経路長に合わせる。 |
| `m_minSegmentDistance` | 頂点を追加する最小移動距離 | 小さいほど滑らかで重い。高速移動で隙間が出ない値にする。 |
| `m_width` | 基本幅 | WidthCurveと掛かる。実寸を先に決める。 |

UVスクロール、距離タイル、ノイズ変位、分岐は既製Trailにはない。必要ならTrail用ShaderまたはComponentを追加する。

## Quad + ModelRenderer

一枚のシルエットを正確に見せる手段。Quadに`ModelRenderer`を付け、ビルボード、Material、Transformを組み合わせる。

| 設定・操作 | 意味 | 選び方 |
| --- | --- | --- |
| `billboardType` | None、CameraFacing、YAxisFixed | 発光核・フラッシュはCameraFacing、地面リングはNone。 |
| `renderPhase` | 透明・Overlayなどの描画段階 | ワールドVFXは通常Transparent、UIはOverlay。 |
| `_MainTex` | 形とAlphaを決めるTexture | 白または明度中心の素材を使い、色はPropertyで変える。 |
| `_BaseColor` | Textureに掛ける色・Alpha | 属性色、フェード、同一素材の色違いを制御する。 |
| `_UVScale` / `_UVOffset` | UVの拡大・切り出し | アトラス切り出しやScriptによるスクロールに使う。 |
| TransformのScale / Rotation | 形の拡縮・回転 | リング拡大、フラッシュ回転、ビーム高さに使う。 |

Quadの寿命、Scale、Alpha、UVは短い専用Scriptで制御する。Soft Particle、深度フェード、画面歪み、Fresnelは既製Shaderにはないため、必要ならShaderを書く。

## Shader・Bloom・UI

- `UnlitAdditive`は`_BaseColor × 頂点Color × _MainTex`を加算する。発光粒子、Trail、フラッシュの第一候補。
- `UnlitTransparent`はAlphaBlend。加算で白飛びする半透明形状に限定して使う。
- Bloomは発光の補助であり、形の代わりにはならない。平常時を抑え、ピークだけが強く届くようにする。
- UIは回復値、Wave通知、TIME警告を担い、ワールドVFXの位置情報を補完する。

## 実装前チェック

1. 何を最初に見せるか。発光核、リング、軌跡のいずれかを核にする。
2. HitStopやDestroy時に、VFX生成、敵破棄、アイテム取得の順序が崩れないか。
3. 既存Textureまたは決定的な生成スクリプトで足りるか。足りない時だけ画像生成を使う。
4. 同時発生してもプレイヤー、敵、回復アイテム、TIMEを隠さないか。
