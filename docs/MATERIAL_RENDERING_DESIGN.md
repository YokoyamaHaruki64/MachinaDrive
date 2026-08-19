# Material・Model 描画設計

## 目的

Shader が宣言する Property と RenderState を基準に、Material、ModelAsset、各 Renderer の責務を分離する。形状は共有し、Material は必要な単位で明示的に生成・割り当てる。

```text
Shaderファイル
  └─ ShaderManager ──所有──> GraphicsShader
                                 ├─ GraphicsShaderPass
                                 └─ ShaderPropertyDesc

MaterialManager ──所有──> Material ──非所有参照──> GraphicsShader
                                ├─ Properties用バイト列 / b3定数バッファ
                                └─ TextureHandle群

ModelManager ──所有──> ModelAsset ──> Mesh群 / embeddedMaterials
                                    └─ ModelNode階層

ModelRenderer ──非所有参照──> ModelAsset
              └─ Material override群
```

## Shader と Pass

`ShaderManager` は `GraphicsShader` を所有する。Shader は Parser と HLSL Reflection の結果から、名前・型・既定値・Storage・offset・size を持つ `ShaderPropertyDesc` と、Pass ごとの `GraphicsShaderPass` を生成する。

`GraphicsShaderPass` は頂点／ピクセルシェーダー、InputLayout、RenderState、入力頂点 Stream、Texture のバインド先を保持し、`Bind()` で GPU 状態を設定する。Pass 名は任意の識別用 Tag であり、無名 Pass も許容する。Material は Pass を所有せず、Shader が持つ Pass 番号または Tag を使って Bind する。

共通 HLSL 定義は `shader/common.hlsl` に置く。定数バッファの予約は次のとおりである。

| Slot | 内容 |
| --- | --- |
| `b0` | PerFrame |
| `b1` | PerObject |
| `b2` | Light |
| `b3` | Material Properties |

共通 Sampler は Renderer が所有し、`LinearSampler` を `s0`、`ShadowSampler` を `s1` に Bind する。Shader Property から宣言する Texture2D は `t16` 以降を使う。

## Material と Property

`Material` は Shader への非所有参照、名前、不変の `MaterialID`、Properties 用バイト列、TextureHandle 群、GPU 定数バッファを持つ。名前は `MaterialManager` 内で一意である。

```cpp
Material* material = MaterialManager::Create("Enemy.Body", *shader);
material->SetTexture("_MainTex", diffuseTexture);
renderer->SetMaterial(0, material);
```

`MaterialManager::Create(name, shader)` が唯一の生成経路である。空名または既存名との重複はエラーとして `nullptr` を返す。`Get(MaterialID)` と `Get(name)` で検索できる。Shader ごとの共有既定値が必要な用途は `GetDefault(shader)` を使い、`Default.<ShaderName>` という名前で作成する。

Renderer は Material を生成しない。Material の設定単位とライフサイクルは、Prefab、Scene、ModelLoader、PostProcess などの利用側が決定し、`SetMaterial` で明示的に割り当てる。オブジェクト固有の Property が必要な場合も、共有 Material を書き換えず別名の Material を作る。`CreateInstance()` は設けない。

Shader Property の値は `MaterialPropertyID` で一意に指定できる。これは上位 32 bit の `MaterialID` と下位 32 bit の `ShaderPropertyID` で構成する。型付き Setter／Getter は名前または ID を受け取り、ID 所有者・型・Storage を検証する。無効な指定は MessageBox を出し、型ごとのエラー値を返す。

定数値は Reflection が決めた offset と size に従ってバイト列へ書き込む。Texture2D など定数バッファ外の値は Resource として別管理する。HLSL 行列は列ごとの 16 byte 境界を考慮してパックする。

## Texture

`TextureManager` はファイルテクスチャと実行時 RenderTexture を `TextureHandle` で一元管理する。Material は Handle のみを持ち、Bind 時に Manager から SRV を取得する。無効 Handle またはロード失敗時は、1x1 の白いフォールバックテクスチャを使う。マゼンタは将来の Material 自体のフォールバック表示用として予約する。

`CreateRenderTexture(name, width, height)` は SRV と RTV を持つ実行時テクスチャを作成する。画面用の Ping-Pong バッファだけは Renderer が所有する。

## ModelAsset と ModelRenderer

`ModelManager` はファイルからロードした ModelAsset と、Quad／Plane／Cube の組み込み Primitive ModelAsset を所有する。`ModelRenderer` は生成時にモデルパスまたは `ModelAsset*` を受け取り、ModelAsset を非所有参照で保持する。`GetModel()` で参照を取得し、`SetModel()` で ModelAsset を差し替えられる。`SetModel()` は旧 Material override を破棄し、新しい ModelAsset の embeddedMaterial slot 数に合わせて Material 構成を初期化する。生成後にモデルを差し替える `Load` API は設けない。

```text
ModelAsset
  ├─ Mesh[]
  │   ├─ VertexBuffer（VertexStreamType ごと）
  │   ├─ IndexBuffer / indexCount / MeshID
  │   └─ materialSlot
  ├─ embeddedMaterials[]
  └─ rootNode（Node階層と Mesh 配置）
```

Assimp ローダーは obj / fbx / gltf を ModelAsset へ直接変換する。aiMesh は 1 Mesh : 1 Material として扱い、Node の参照とローカル変換を保持する。Position は必須であり、欠けた Mesh は描画時にエラーとする。その他の要求 Stream は Renderer のフォールバックバッファで補う。Normal が存在して Tangent がない場合、Loader は Tangent を生成する。

`embeddedMaterials` はファイルまたは組み込み ModelAsset の既定 Material である。`ModelRenderer::SetMaterial(slot, material)` による override があればそちらを優先し、`nullptr` を設定すると embeddedMaterial へ戻る。Quad／Plane／Cube の組み込み Primitive は、初期状態で共通の DefaultLit Default Material を slot 0 に使う。テクスチャ、透明、加算、オブジェクト固有 Property が必要な利用側だけ、名前付き Material を明示生成して override する。

Sprite と Trail は ModelAsset を持たない。Sprite は 2D 用の不変頂点バッファ、Trail は毎フレーム更新する動的頂点バッファを直接保持する。Particle は共有 Quad Mesh を参照する専用 Renderer である。

### ModelLoader

`ModelLoader` は Assimp を使い、obj / fbx / gltf を中間表現を介さず `ModelAsset` へ変換する。`ModelManager::Load(filePath)` がキャッシュを確認してから Loader を呼び、同じパスの ModelAsset を共有する。

ロード時には `aiProcess_ConvertToLeftHanded`、三角形化、頂点結合、スムーズ法線生成、プリミティブ種別のソートを適用する。したがって、エンジン内の座標系は左手系であり、UV 原点は左上を `(0, 0)` とする。個別のモデルだけ向きを補正する場合は、`ModelRenderer` の `InverseAxis` を使う。指定した軸には描画時に π 回転を合成するため、頂点座標や三角形の表裏は変更しない。

aiMesh ごとに次の Vertex Stream を生成する。存在しない属性は Stream を作らず、描画時に Shader の要求に応じてフォールバックを使う。

| Assimp 属性 | VertexStreamType | 備考 |
| --- | --- | --- |
| Position | Position | 必須。欠落時は描画しない。 |
| Normal | NormalTangent | Normal と Tangent をまとめて保持する。 |
| Tangent / Bitangent | NormalTangent | Tangent は `Vector4`。w は接空間の向きに使う。 |
| TextureCoords[0] | PrimaryTexCoord | `Vector4` として保持する。 |
| TextureCoords[1] | SecondaryTexCoord | 存在時のみ保持する。 |
| TextureCoords[2] 以降 | ExtraTexCoord | 現在は必要な拡張時に利用する。 |
| Vertex Color[0] | Color | 存在時のみ保持する。 |

Normal が存在して Tangent がない場合は、UV0 と三角形の位置・法線から Tangent を生成する。Position 以外の不足 Stream は Renderer の既定頂点バッファで補えるが、Position は形状を復元できないためフォールバックしない。

Material は aiMaterial ごとに `DefaultLit` 用の名前付き Material として生成する。名前はモデルパス、Material index、Assimp 側の Material 名から構成する。Diffuse／Emissive 色、Shininess、対応するテクスチャを Shader Property へ設定する。テクスチャは Assimp の埋め込みデータ（`*0` 形式を含む）を先に解決し、なければモデルファイルからの相対パスを `TextureManager` へ渡す。解決できない場合は Material Bind 時に白いテクスチャフォールバックが使われる。

Node 階層は `ModelNode` として再帰的に保存し、各 Node は参照する Mesh index とローカル変換を持つ。`ModelRenderer` は描画時に Node を再帰走査し、Node のローカル変換とコンポーネントの World 変換を合成して各 Mesh を描画する。Animation、Skeleton、Skinning はまだロード・保持しない。

## 通常描画とソート

すべての `RendererBase` は `Submit()` で `DrawItem` を提出する。現在の DrawItem は Renderer、sortKey、代表 MeshID、提出順を持つ。`Renderer::Draw()` は Phase ごとに sortKey と提出順で安定して並べ、各 Renderer の `Draw()` を呼ぶ。

| Phase | 主なソート順 | 方針 |
| --- | --- | --- |
| Opaque | MaterialID → MeshID → drawLayer | GPU 状態変更を抑える。 |
| Transparent | drawLayer → 奥から手前の深度 → MaterialID / MeshID | 合成順を優先する。 |
| Overlay | drawLayer → MaterialID → MeshID | 2D の明示順を優先する。 |

Renderer が Mesh を持たない場合、MeshID は `0` とする。ModelRenderer は複数 Mesh を描けるため、現状の提出ソートでは先頭 Mesh を代表値として利用する。Mesh／Material／Pass 単位への DrawItem 分割と Bind キャッシュは次段階の最適化対象である。

## PostProcess

`PostProcess` は Material を使うが通常描画キューへは提出しない。画面入力 `PostProcessSource` は HLSL の共通 API として `t0` に Bind する。Pass は番号または Tag で指定して実行できる。

画面出力時は Renderer 所有の Ping-Pong バッファへ描画し、成功時だけ入出力を交換する。明示した RenderTexture へ出力する場合は、Renderer の画面入力を交換しない。`Renderer::SetRenderTarget` は RenderTarget の設定と、そのテクスチャサイズに合わせた Viewport 設定をまとめて行う。

## ライフサイクル

初期化順は Renderer、TextureManager、ShaderManager、MaterialManager、ModelManager とする。終了時は GameObject と PostProcess を先に破棄し、ModelManager、MaterialManager、TextureManager、ShaderManager、Renderer の順に解放する。これにより、非所有参照を持つ ModelAsset と Material を、参照先の Shader／GPU デバイスより先に解放する。

## 今後の拡張

- Mesh／Material／Pass 単位の DrawItem 化と Bind キャッシュ
- Skeleton、Animation、Skinning の ModelAsset への追加
- TextureCube、Texture2DArray、Buffer 系 Resource Property
- RenderPhase::Shadow の実描画キュー
- Primitive Material の生成場所を Prefab／Scene から Asset 定義へ整理
