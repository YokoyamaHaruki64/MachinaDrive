# UIシステム設計

## 目的と初期範囲

ゲームUIを解像度に依存せず配置し、Inspectorでレイアウトを調整しながら、Scriptでは演出値を直感的な数値で扱えるようにする。初期実装はScreen Space Overlayの画像、Text、Button、単一Pointer入力を対象とし、リッチテキスト、ゲームパッドナビゲーション、World Space UIは後続とする。

```text
UICanvas
  └─ UITransform
       └─ UIBehaviour
            ├─ UIImage
            ├─ UIText
            └─ UIButton
```

通常の3D／2D GameObjectは`Transform`を持つ。UI専用GameObjectは`UITransform`を持ち、UI要素は同じCanvasに属する`UITransform`だけを親にできる。これによりUI座標系とWorld座標系を混在させない。

## Canvasと座標系

Canvasの基準解像度は`1920 x 1080`、基準アスペクト比は16:9とする。実際のGame RenderTargetが16:9以外の場合は、その内部に収まる最大の16:9領域をCanvas領域として中央配置する。UIを画面全体へ非一様に引き伸ばさない。

```text
canvasScale = min(renderWidth / 1920, renderHeight / 1080)
canvasOffset = (renderSize - (1920, 1080) * canvasScale) / 2
```

レイアウトの正は比率、演出の正はCanvas基準単位とする。

| 種別 | 単位 | 用途 |
| --- | --- | --- |
| Layout | 親Rectに対する比率 | Anchor、Size、Layout Offset、Pivot |
| Animation | 1920 x 1080基準単位 | 揺れ、Tween、移動量 |
| Computed | 実RenderTargetのpx | 描画、入力判定、表示確認 |

例えば`Offset = (0, 15)`は、1920 x 1080では15px、1280 x 720では10px、3840 x 2160では30pxとして描画する。

## UITransform

`UITransform`は`TransformBase`派生のComponentとしてUI専用の比率LayoutとWorld行列を所有する。通常の`Transform`のLocal TRSやWorldキャッシュは共有せず、親子関係は`TransformBase`が一元管理する。通常Objectは`Manager::CreateEmpty()`で作成し、UIは専用の生成経路で最初から`UITransform`を生成する。

```cpp
GameObject* canvas = Manager::CreateUICanvas("Canvas");
GameObject* image = Manager::CreateUIObject("Logo");
image->SetParent(canvas, ParentTransformMode::KeepLocal);
```

ロード時もTransformの種類を先読みして適切な型のGameObjectを生成する。通常の`Transform`を生成した後に`UITransform`へ置換してはならない。

### Inspectorで編集するLayout値

- `Anchor`: 親の基準位置（`LeftUp`〜`RightBottom`の9点）
- `Pivot`: 自身の基準位置（`LeftUp`〜`RightBottom`の9点）
- `Stretch`: 親Rectに対するサイズ比率
- `Pos`: Anchorからの親Rectに対する位置比率
- `Lock Aspect Ratio`: 有効時はUIImageのテクスチャ比率を基準にStretchの片軸を補正

### Scriptで操作する演出値

Scriptは実画面のサイズやCanvas Scaleを意識しない。配置を直接変更せず、演出専用の値を設定する。

```cpp
uiTransform->SetOffset(Vector2(0.0f, 15.0f));
uiTransform->AddOffset(Vector2(0.0f, 15.0f));
uiTransform->SetUIRotation(0.1f);
uiTransform->AddScale(Vector2(0.1f, 0.1f));
```

`Offset`はCanvas基準単位、RotationはUI平面のZ回転、ScaleはStretchへ掛ける表示倍率とする。ScaleとOffsetはInspectorへ公開しない。最終RectとUI World行列はCanvasが導出し、UIImage／UITextがRendererへ設定する。

実画面上の値が必要な場合は、意味を明示したGetterを使う。

```cpp
Vector2 center = uiTransform->GetCenterPixels();
Vector2 size = uiTransform->GetSizePixels();
UIRect rect = uiTransform->GetPixelRect();
```

Inspectorは`Layout`、`Animation`、読み取り専用の`Computed`を分けて表示する。

## UIBehaviourとPointer入力

`UIBehaviour`はUI Component共通の基底とし、所有GameObjectのUITransformと親Canvasを非所有参照として取得する。Canvasは親UITransformをたどって解決する。親子付け替えでも古いCanvasを参照しないよう、初期実装では必要時に再解決する。

```cpp
UICanvas* canvas = GetCanvas();
Vector2 pointer = canvas->GetPointerPosition();

if (WasPointerPressed()) { /* Pointer開始 */ }
if (IsPointerPressed()) { /* Pointer継続 */ }
if (WasPointerReleased()) { /* Pointer終了 */ }

if (WasDragStarted()) { /* Drag開始 */ }
if (IsDragging()) { /* Drag継続 */ }
if (WasDragEnded()) { /* Drag終了 */ }
```

Canvasはマウス座標をCanvas座標へ変換し、描画順の逆順で`Raycast Target`が有効なUI要素をRect Hit Testする。初期実装はマウス左ボタン・単一Pointer・矩形Hit Testだけを扱う。

- Pointer Downした要素は、Rect外へ出てもPointer UpまでCaptureする。
- `Drag Enabled`が有効な要素だけ、Pointer移動がDrag開始しきい値を超えるとDrag状態へ移行する。既定値は無効とする。
- Drag後はButton Clickを発火しない。
- Overlayの最前面にある要素だけをHit対象とする。

PointerとDragのAPIは、開始フレームだけtrueになる`Was...`、継続中にtrueになる`Is...`、終了フレームだけtrueになる`Was...Ended`へ分ける。`Drag Enabled`が無効でもPointer APIは利用できる。UIButtonはPointer開始・継続・終了を公開し、Pointer終了時にDragへ移行していなければ`IsClickedThisFrame()`をtrueにする。

### UIImageとUIButtonの操作

UIImageは画像表示と基本的なPointer操作を兼ねる。`Raycast Target`が有効なUIImageは、Scriptからドラッグ可能なアイコン、カード、アイテム枠などとして直接利用できる。初期版では画像の透明度を見ない矩形Hit Testとする。

UIButtonはUIImageを置き換えず、同じGameObjectへ追加するクリック用Componentとする。UIImageのPointer状態から`Normal`、`Hovered`、`Pressed`、`Disabled`を判定し、`IsClickedThisFrame()`を公開する。初期版の視覚フィードバックは色変更だけとし、状態ごとの画像差し替えは後続とする。

```text
GameObject
  ├─ UITransform
  ├─ UIImage      # 描画・Hit Test
  └─ UIButton     # Click判定・状態別の表示
```

UITextの`Raycast Target`は初期値を無効とする。Text自体を操作対象にする時だけ明示的に有効化する。

### UI同士の重なり問い合わせ

UICanvasはPointer Hit Testとは別に、UIRect同士の重なりを問い合わせるAPIを提供する。

```cpp
std::vector<GameObject*> targets =
    canvas->GetOverlappingGameObjects(*draggedUITransform);
```

対象は同じCanvasに属する有効なUI要素とし、自分自身、無効Object、破棄予約Objectは除外する。導出済みpx RectのAABBで判定し、辺が接するだけは重なりとして扱わない。返却順は前面から背面とする。

Scriptは返却されたGameObjectから必要なComponentだけを選び、アイテム入替などのゲーム固有処理を行う。

```cpp
for (GameObject* gameObject : targets)
{
    if (InventorySlot* slot = gameObject->GetComponent<InventorySlot>())
    {
        slot->TryAccept(item);
        break;
    }
}
```

## UIImageとTexture Binding

UIImageは共有MaterialとSpriteを持つ。MaterialはShader、RenderState、定数Propertyなど要素間で共有する値だけを持ち、UIImageごとに異なるTextureはRenderer側が保持する。

```text
Material
  └─ Shader / Blend / Cull / 定数Property

UIImage
  └─ Sprite（初期実装ではTextureHandle）
       └─ TextureManagerからSRVを解決して直接Bind
```

Texture実体とGPUリソースは`TextureManager`が所有する。UIImageが保持するTextureHandleは非所有の参照であり、Scene／PrefabにはTextureのアセット相対パスだけを保存する。

Sprite用のTextureはMaterial Propertyにせず、Rendererが予約スロットへ直接Bindする。`t1`はBloomで使用済みのため、初期予約は`t2`とする。

| Slot | 内容 | 所有者 |
| --- | --- | --- |
| `t0` | 既存Shader／PostProcess入力 | 各既存描画経路 |
| `t1` | Bloom用Texture | PostProcess |
| `t2` | `SpriteTexture` | UIImage、UIText、将来のSpriteRenderer |
| `t16`以降 | Material PropertyのTexture2D | Material |

UI Shaderは次のように宣言する。

```hlsl
Texture2D SpriteTexture : register(t2);
SamplerState LinearSampler : register(s0);
```

描画ではMaterialをBindした後にRenderer側が`SpriteTexture`をBindする。Overlay描画後はSRVを解除し、将来RenderTextureをSpriteとして使った時もRTVとの読み書き競合を残さない。

この方式は初期段階ではUIImageとUITextだけに導入する。既存SpriteRendererのMaterial Property方式は変更せず、必要になった時点で同じSprite Binding方式へ移行する。

## FontAssetとUIText

Textは`FontManager`が所有・キャッシュする`FontAsset`を使う。FontAssetはBMFontテキスト形式の`.fnt`と、同名・同階層のatlas PNGを1つのアセットとして扱う。

```text
Assets/Fonts/
  NotoSansJP_64.fnt
  NotoSansJP_64.png
```

```text
FontAsset
  ├─ .fntのパス
  ├─ atlas PNGのパス（.fntから自動導出）
  ├─ atlas TextureHandle
  ├─ lineHeight / baseLine
  ├─ Glyph情報（Unicode、UV、offset、advance）
  └─ Kerning情報
```

UITextはFontAssetを非所有参照し、Scene／Prefabには`.fnt`のパスだけを保存する。Inspectorは`.fnt`だけを選択可能とし、PNGを個別指定させない。対応するPNGがない場合はFontAssetをエラー状態として表示する。

TextのatlasはUIImageと同じ`SpriteTexture (t2)`へBindする。UITextは文字列全体を1つの動的頂点バッファとして描画し、GlyphごとにDraw Callを増やさない。

初期機能は単一／複数行Text、横揃え、縦揃え、折返し、行間、色、Canvas基準単位のFont Sizeとする。存在しないGlyphは`?`または□へフォールバックし、初回だけ警告を記録する。

SDF／MSDFはFontAssetの公開責務を変えずに後から追加する。初期実装では依存追加を避け、BMFontテキスト形式のみを対象とする。

## 初期実装の完了条件

1. `UICanvas`と`UITransform`が16:9のCanvas領域と比率Layoutを導出できる。
2. `UIImage`が共有Materialと要素ごとの`SpriteTexture`を使って描画できる。
3. `FontManager`と`FontAsset`が`.fnt`と同名PNGを読み込み、`UIText`が描画できる。
4. UIImageのRect Hit Test、Pointer／Dragの開始・継続・終了、Pointer Capture、UIButtonのClick判定が動作する。
5. UIのLayout、Texture Path、Font Path、TextをInspectorで編集し、Scene／Prefab保存後に復元できる。
6. UICanvasがUI要素の重なりをGameObject一覧として返し、ScriptからDrop先を判定できる。
7. Title／Resultの既存画像UIをUIImageへ移行し、Scriptから`Offset`で揺れを付けられる。

汎用イベントのシリアライズ、ゲームパッドナビゲーション、9-slice、Mask、SDF／MSDF、World Space Canvas、既存SpriteRendererの移行は初期実装に含めない。
