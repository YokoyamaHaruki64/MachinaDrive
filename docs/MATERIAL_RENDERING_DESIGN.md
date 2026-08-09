# Material描画・Phase別キュー設計

## 目的

実行時に読み込んだ `GraphicsShader` と `Material` を使い、描画対象を `RenderPhase` ごとに収集、ソート、一括実行する。これにより、不透明描画ではGPU状態の切替を抑え、透明描画では正しい合成順を維持する。

## 全体構成

```text
ModelRenderer
  └─ MaterialごとのGraphicsShaderPass
       └─ Renderer::Submit(DrawItem)

Renderer
  └─ queues[RenderPhase::Count]
       └─ DrawItemをPhaseごとにソート・一括実行
            ├─ GraphicsShaderPassのBind
            ├─ MaterialのBind
            ├─ MeshのBind
            └─ 描画呼び出し
```

`ModelRenderer` はモデルの各サブセットとMaterialの各 `GraphicsShaderPass` の組合せごとに `DrawItem` を提出する。Shader、Material、RenderStateの適用と、直前の描画との差分を利用したBind省略は `Renderer` が担う。

## DrawItem

`DrawItem` は1回の実描画を表すフレーム内限定の提出データとする。少なくとも次の情報を持つ。

| 項目 | 用途 |
| --- | --- |
| `sortKey` | Phase内の描画順を表す64ビットキー |
| `renderer` | ワールド行列設定と実Drawを担う提出元 |
| `material` | Properties定数バッファ、Texture、SamplerのBind対象 |
| `meshID` | 頂点・インデックスバッファ組を識別する不変ID |
| `shaderPass` | Shader、InputLayout、Pass固有RenderStateのBind対象 |
| `submitOrder` | 同一キー時の順序を保証する連番 |

`MaterialID` はMaterial生成時に不変で採番する。`MeshID` も `MODEL` またはサブセットのGPUバッファ組に不変IDを付与する。MeshIDが未実装の間は `0` を使い、ポインタ値を永続IDとして扱わない。

提出データは同一フレーム内でのみ有効であり、`Renderer::Draw()` の完了後に各キューをクリアする。

## ModelAsset・Mesh・MaterialSlot

モデルの共有可能な形状データと、GameObjectごとに変更されるMaterialを分離する。

```text
ModelAsset
  └─ Mesh
       ├─ VertexStreams（Position / NormalTangent / TexCoord …）
       ├─ IndexBuffer / MeshID
       └─ SubMesh（indexStart / indexCount / materialSlot）

ModelRenderer
  ├─ ModelAssetへの共有参照
  └─ MaterialSlotごとのMaterialインスタンス
```

`Mesh` は `GraphicsShaderPass` が要求する頂点ストリームを対応するInput SlotへBindする。`SubMesh` は頂点・Materialを所有せず、index範囲と `materialSlot` の対応だけを保持する。`ModelRenderer` はSubMeshが指定するMaterialから各ShaderPassのDrawItemを提出する。

OBJの移行初期は Position、Normal、TexCoord0 をストリーム化する。tangent は必要なShaderを導入する時点でロード時に生成する。既存のinterleaved `VERTEX_3D` は互換経路として残し、すべての既存シーンがMaterial経路へ移行した後に削除する。

## Phase別キューとソート

`Renderer` は `std::array<std::vector<DrawItem>, RenderPhase::Count>` 相当のPhase別キューを持つ。Phaseは外側のキューで分離するため、`sortKey` にPhaseビットは格納しない。

| Phase | ソート優先順位 | 方針 |
| --- | --- | --- |
| `Shadow` | Pass/State → MaterialID → MeshID → submitOrder | 状態切替削減を優先する。シャドウ用レンダーターゲット導入時に有効化する。 |
| `Opaque` | drawLayer → Pass/State → MaterialID → MeshID → submitOrder | 不透明描画のバッチングを優先する。 |
| `Transparent` | drawLayer → カメラ距離（奥→手前）→ submitOrder | 正しい合成順を優先する。MaterialIDやMeshIDを距離より前に置かない。 |
| `PostProcess` | 専用実行順 | 現行の `PostEffect` 経路を維持し、通常のDrawItemキューには入れない。 |
| `Overlay` | drawLayer → submitOrder | UI・スプライトの明示的な前後関係を維持する。 |

透明描画で距離が同じ場合に限り、将来 `MaterialID` と `MeshID` を距離より後ろのタイブレークとして使用できる。ただし、描画順を変えないことを優先する。

`drawLayer` は符号付き値をそのままビットシフトせず、定義済みの範囲へ正規化してキーへ格納する。キーが同一の場合も再現性を保つため、`submitOrder` を末尾の比較要素とする。

## GPU状態の適用

### MaterialのShader所有とBind API

`Material` は共有アセットである `GraphicsShader` を非所有参照として保持し、Propertiesの値、Properties用定数バッファ、Texture、Samplerを所有する。Shader自体の寿命は `ShaderManager` が管理する。

複数Passに対応するため、Materialは外部の `GraphicsShaderPass` を受け取らず、Shader内のPass番号で操作する。

```cpp
const GraphicsShaderPass& Material::GetShaderPass(size_t passIndex) const;
void Material::Bind(size_t passIndex);
```

RendererはPassをBindした後、同じPass番号でMaterialをBindする。PassはShader・InputLayout・RenderStateを、Materialは反射結果に基づく `b7` のProperties定数バッファとTexture/Samplerを設定する。

描画実行時は直前にBindした `GraphicsShaderPass`、Material、Meshをキャッシュし、同一であれば再Bindしない。

`GraphicsShaderPass` は頂点・ピクセルシェーダー、InputLayout、Pass固有の `RenderState` を適用する。`Material` は反射情報に従い、Material Properties用定数バッファ、Texture、Samplerを各シェーダーステージへ設定する。Meshは頂点・インデックスバッファとトポロジを設定する。

深度状態は `RenderState::depthTestMode` と `RenderState::depthWriteMode` を個別に反映する。既存の `Renderer::SetDepthEnable()` は深度書込みを止める用途も含むため、新しい経路ではこれに依存せず、必要なD3D深度ステンシルステートを作成・キャッシュする。

## 既存描画との移行

初期段階では既存の `RendererBase::Submit()` も互換 `DrawItem` としてPhase別キューへ投入する。既存の `RenderPass` は次のように `RenderPhase` へ対応付ける。

| 既存 `RenderPass` | 移行先 `RenderPhase` |
| --- | --- |
| `Opaque` | `Opaque` |
| `Transparent` | `Transparent` |
| `Overlay` | `Overlay` |

これにより、Sprite、Trail、Particleなどの既存Rendererを維持したまま、まず `ModelRenderer` のMaterial描画を新しい `DrawItem` 経路へ接続する。その後、既存Rendererも必要に応じて新しいMaterial経路へ移行する。

## 実装順序

1. `renderer.h/.cpp` に `DrawItem` とPhase別キューを導入し、既存 `RendererBase` の互換提出を通す。
2. Phase別ソートと `submitOrder` による安定した実行順を実装する。
3. `GraphicsShaderPass` のBindと `RenderState` の適用・キャッシュを実装する。
4. `Material` のProperties・Texture・SamplerのBindを実装する。
5. `ModelRenderer` がサブセット × PassごとのDrawItemを提出するよう移行する。
6. `MeshID` を導入し、不透明・Shadowのバッチングを有効化する。

## 受け入れ条件

- 既存のOpaque、Transparent、Overlay描画が従来と同じ順序で表示される。
- 同一Shaderを共有する複数Materialが、それぞれ異なるProperties・Textureで描画できる。
- Opaqueでは同一Pass、Material、Meshの不要な再Bindが発生しない。
- TransparentではMaterialやMeshが異なっても、カメラから奥のものが先に描画される。
- 同一ソート条件の提出順がフレームごとに変化しない。
