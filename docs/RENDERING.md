# 描画システム

## 目的と範囲

本資料は、DirectX 11 を用いる現在の描画基盤をまとめる。対象は Renderer、Shader、Material、Texture、ModelAsset、各描画 Component、および PostProcess である。ゲーム固有の描画内容ではなく、これらを利用する際の責務とデータの流れを正とする。

```text
Shader ファイル ──> ShaderManager ──所有──> GraphicsShader
                                          ├─ Pass / RenderState / InputLayout
                                          └─ ShaderPropertyDesc

MaterialManager ──所有──> Material ──非所有参照──> GraphicsShader
                              ├─ b3 用 Property バイト列
                              └─ TextureHandle 群

ModelManager ──所有──> ModelAsset ──> Mesh 群 / embeddedMaterials / Node 階層
RendererBase ──提出──> Renderer の Phase 別描画キュー
PostProcess ──> Material と Renderer 所有の画面 Ping-Pong バッファ
```

## フレームの描画フロー

`Manager::Draw()` は Game RenderTarget の開始後、各 `RendererBase` の `Submit()` を呼ぶ。`Renderer` は Phase ごとにキューを安定ソートし、各 Component の `Draw()` を実行する。PostProcessはMaterialの先頭Shader Passの `RenderPhase` と一致するPhaseで実行する。Editor構成では全Phaseの後にBackBuffer RTVへ切り替えてImGuiを描画し、Runtime構成ではGame RenderTargetをBackBufferへコピーしてPresentする。

```text
Renderer::Begin
  └─ RendererBase::Submit
       └─ Phase 別 DrawItem キュー
            └─ Shadow / Opaque / Transparent
                 └─ PostProcess の PostProcess Pass
                      └─ Overlay
                           └─ PostProcess の FinalPostProcess Pass
                                ├─ Editor: BackBuffer RTV / GameWindow::DrawImGui / Renderer::End
                                └─ Runtime: CopyResource / Renderer::End
```

`DrawItem` は Renderer、sortKey、代表 MeshID、提出順を持つ。提出順は同じ sortKey の安定した順序を保つために使う。現在のキュー単位は Renderer であり、ModelRenderer 内の複数 Mesh や Material Pass へはまだ分割しない。

### RenderPhase とソート

| Phase | ソート順 | 目的 |
| --- | --- | --- |
| `Opaque` | MaterialID → MeshID → drawLayer | GPU State と頂点バッファの切替を抑える。 |
| `Transparent` | drawLayer → 奥から手前の深度 → MaterialID / MeshID | 正しいアルファ合成順を優先する。 |
| `PostProcess` | PostEffectの登録順 | Overlay前の画面へフルスクリーン処理を適用する。 |
| `Overlay` | drawLayer → MaterialID → MeshID | 2D の明示的な重なり順を優先する。 |
| `FinalPostProcess` | PostEffectの登録順 | Overlayを含む画面へフルスクリーン処理を適用する。 |

Mesh を持たない Renderer の MeshID は `0` とする。ModelRenderer は複数 Mesh を描画できるため、現状は先頭 Mesh を代表値として提出する。より細かな State 最適化が必要になった時点で、Mesh / Material / Pass 単位の DrawItem 化と Bind キャッシュを導入する。

## Renderer と共通 GPU 状態

`Renderer` は D3D11 Device、Context、SwapChain、BackBuffer RTV、可変サイズのGame用Ping-Pongバッファ、共通定数バッファ、Sampler、Depth / Blend / Rasterizer State を所有する。Depth State は `DepthTestMode` と `DepthWriteMode` の全組み合わせを初期化時に生成・キャッシュし、描画中に生成しない。

共通 HLSL 定義は `shader/common.hlsl` に置く。予約スロットは次のとおりである。

| Slot | 内容 | 所有者 |
| --- | --- | --- |
| `b0` | PerFrame（View、Projection、CameraPosition、Time） | Renderer |
| `b1` | PerObject（World） | Renderer |
| `b2` | Light | Renderer |
| `b3` | Material Properties | Material |
| `b4` | Shadow（LightViewProjection、ShadowMap情報） | Renderer |
| `s0` | LinearSampler | Renderer |
| `s1` | ShadowSampler | Renderer |
| `t1` | Directional ShadowMap | Renderer |
| `t16` 以降 | Material Property の Texture2D | Material |

頂点属性は `VertexStreamType` ごとの独立バッファで扱う。Shader Pass が要求する Stream が Model にない場合、Position 以外は Renderer の既定頂点バッファで補う。Position は形状を定義する必須属性なので補完せず、描画側でエラーとする。

## Shader と Material

`ShaderManager` は `GraphicsShader` を所有する。Shader Loader は Shader ファイルの Parser と HLSL Reflection の結果から、Pass と `ShaderPropertyDesc` を構築する。Property Desc は ID、名前、表示名、型、既定値、Storage、定数バッファ内 offset / size を持つ。offset / size は C++ 構造体ではなく Reflection の結果を正とする。

`GraphicsShaderPass` は頂点 / ピクセル Shader、InputLayout、RenderState、入力 Stream の対応、定数バッファ情報、Texture Binding 先を持つ。Pass 名は任意の識別 Tag であり、無名 Pass も許容する。Material は Shader が所有する Pass を番号または Tag で Bind する。

`Material` は Shader への非所有参照、不変の `MaterialID`、一意な名前、Property の実データ、TextureHandle、b3 用 GPU 定数バッファを持つ。Renderer は Material を生成せず、Prefab、Scene、ModelLoader、PostProcess などの利用側が `MaterialManager` を通じて明示生成する。

```cpp
Material* material = MaterialManager::Create("Enemy.Body", *shader);
material->SetTexture("_MainTex", diffuseTexture);
renderer->SetMaterial(0, material);
```

`MaterialManager::GetDefault(shader)` は Shader ごとの共有既定 Material を返す。オブジェクト固有の値が必要な場合は共有 Material を変更せず、別名の Material を生成して割り当てる。`CreateInstance()` は持たない。

Property は定数バッファへ入る値と Resource に分かれる。現在、`Texture2D` は Resource、それ以外は定数バッファ値である。`MaterialPropertyID` は上位 32 bit の MaterialID と下位 32 bit の ShaderPropertyID で構成し、名前または ID 指定の型付き Setter / Getter を提供する。ID 所有者、型、Storage が不正な場合は MessageBox を表示し、型ごとのエラー値を返す。

## Texture と RenderTexture

`TextureManager` はファイル Texture、メモリから読み込む Texture、実行時 RenderTexture を `TextureHandle` で一元管理する。Material は Handle のみを保存し、Bind 時に Manager から SRV を取得する。無効 Handle またはロード失敗時には 1x1 の白いフォールバック Texture を使う。マゼンタは将来の Material 自体のフォールバック表示用に予約する。

`CreateRenderTexture(name, width, height)` は SRV と RTV を持つ Texture を生成する。同名RenderTextureへ異なるサイズを要求した場合は、既存Handleを維持したままGPUリソースを置き換える。`Renderer::SetRenderTarget()` は TextureHandle から RTV を解決し、RenderTarget サイズに合わせて Viewport も設定する。画面用 Ping-Pong バッファは TextureManager ではなく Renderer が所有する。

Game Viewの表示領域が変わると、RendererはGame RenderTargetのPing-PongテクスチャとDepthStencilを再生成する。3D Cameraは現在のGame RenderTargetからアスペクト比を求め、Bloomなどの中間RenderTextureも同じサイズ変更通知を受ける。2D描画の射影も現在のGame RenderTargetサイズを使用し、`SCREEN_WIDTH`／`SCREEN_HEIGHT`は初期値・フォールバックとしてのみ使用する。

## ModelAsset と ModelLoader

`ModelManager` はファイル由来 ModelAsset と、Quad / Plane / Cube の組み込み Primitive ModelAsset を所有する。`ModelManager::Load(filePath)` は同じパスの Asset をキャッシュして共有する。

```text
ModelAsset
  ├─ Mesh[]
  │   ├─ VertexStreamType ごとの VertexBuffer
  │   ├─ IndexBuffer / indexCount / MeshID
  │   └─ materialSlot
  ├─ embeddedMaterials[]
  └─ rootNode（ローカル変換、Mesh index、子 Node）
```

`ModelLoader` は Assimp を使用し、`.obj`、`.fbx`、`.gltf` を中間表現なしで ModelAsset に変換する。左手系変換、三角形化、頂点結合、スムーズ法線生成、プリミティブ種別ソートを適用する。UV 原点は左上 `(0, 0)` とする。

Assimp の aiMesh は 1 Mesh : 1 Material として取り込み、Material slot と Node 階層を保持する。Position は必須である。Normal があり Tangent がない場合は、UV0、位置、法線から Tangent を生成する。Texture は Assimp の埋め込みデータを優先し、なければモデルファイルからの相対パスとして解決する。Animation、Skeleton、Skinning はまだ保持しない。

### ModelRenderer

`ModelRenderer` はモデルパスまたは `ModelAsset*` を受け取り、Asset を非所有参照する。`SetModel()` は現在の Material override を破棄し、新しい Asset の embedded Material slot 数へ同期する。生成後にパスから差し替える `Load` API は持たない。

Mesh の `materialSlot` は `embeddedMaterials` を参照する。`SetMaterial(slot, material)` による override があればそれを優先し、`nullptr` なら embedded Material へ戻す。組み込み Primitive は初期状態で共通の DefaultLit Material を使う。

Node のローカル変換と Component の World 変換を描画時に合成して各 Mesh を描画する。`InverseAxis` は座標の鏡映ではなく、指定軸に π 回転を合成してモデルの向きを補正する。頂点座標や三角形の表裏は変更しない。

`castShadow` と `receiveShadow` は ModelRenderer 単位の設定で、どちらも既定値は有効である。Shader Pass の `RenderPhase = Shadow` はShadowMapへの描画を表し、`castShadow` が無効なModelはそのPhaseでは描画されない。Shadow Passを持たないShaderは影を落とさない。受影可否はPerObject（b1）へ渡され、共通の `lighting.hlsl` は `receiveShadow` が無効なら影係数1を即座に返す。

RendererはShadow Phaseの開始時だけ描画先を4カスケード×2048²のShadowMap深度配列へ切り替え、終了時にGame RenderTargetと通常Viewportへ復帰する。カスケードはカメラ深度 `0.1–10 / 10–25 / 25–55 / 55–100m` を担当し、各カスケードでライト空間のテクセルスナップを行って移動時の揺れを抑える。受影Shaderは視点深度からカスケードを選択し、分割境界の10%を隣接カスケードと補間する。Shadow PassのRasterizerは小さい固定Depth BiasとSlope-Scaled Depth Biasを併用し、DefaultLitなど法線を持つShadow Passは `lighting.hlsl` の最小限の法線方向オフセットを適用する。受影側の比較バイアスも微小値に保つ。ライト空間変換、深度比較、5×5 PCFは `lighting.hlsl` に共通化する。Opaque用Shadow Passは深度のみ、Cutout用Shadow PassはShader自身が同じalpha clipを行い、透過・エフェクト等はShadow Passを定義しないことで対象外にする。ShadowCasterModeのような別設定は設けない。

Sprite と Trail は ModelAsset を持たない。Sprite は 2D 用の不変頂点バッファ、Trail は動的頂点バッファを直接保持する。Particle は共有 Quad Mesh を参照する専用 Renderer である。

## PostProcess

PostProcess は通常描画キューへ提出せず、Material を使ってフルスクリーン Quad を実行する。実行PhaseはPostProcess自身では保持せず、Materialの先頭Shader Passの `RenderPhase` で決まる。`PostProcess` を指定するとOverlay前、`FinalPostProcess` を指定するとOverlay後に実行する。PostProcess ごとの値は専用 Material に保持するため、通常描画と同じ Property API を利用できる。

画面入力 `PostProcessSource` は Material Property ではなく、全 PostProcess 共通の `t0` に Bind する。Pass は番号または Tag で実行できる。画面出力では Renderer 所有の Ping-Pong バッファの出力先へ描画し、成功時にのみ入出力を交換する。明示した RenderTexture への出力は画面入力を交換しない。Pass 終了時には設定した SRV を解除し、次の RenderTarget との読み書き競合を防ぐ。

## 初期化・終了順

初期化は Renderer、TextureManager、ShaderManager、MaterialManager、ModelManager の順に行う。終了時は GameObject と PostProcess を先に破棄し、ModelManager、MaterialManager、TextureManager、ShaderManager、Renderer の順に解放する。これにより、Material / ModelAsset が参照する Shader と GPU リソースの寿命を守る。

## 今後の課題

描画、ModelAssetを含む将来の拡張・最適化項目は[FUTURE_WORK.md](FUTURE_WORK.md)を参照する。
