# アーキテクチャ

## 概要

Project_Drive は、Windows / DirectX 11 を基盤とするコンポーネント指向のゲームフレームワークである。アプリケーションループは `source/System/Core/main.cpp` にあり、`Manager` がシーン、ゲームオブジェクト、描画、ポストエフェクトの進行を統括する。

```text
WinMain
  ├─ Manager
  │    ├─ Scene
  │    │    └─ GameObject
  │    │         └─ Component
  │    ├─ Renderer / RendererBase
  │    ├─ Physics / Jolt Physics
  │    ├─ PostEffect
  │    └─ Prefab archive / legacy factory
  ├─ EditorSystem（Debug構成のみ）
  │    ├─ Game / Hierarchy / Inspector
  │    └─ Performance / Log
  └─ 診断システム
       ├─ Logger / LogSystem / LogFileWorker
       ├─ PerformanceMonitor
       └─ PerformanceWindow / LogWindow
```

## モジュール構成

| 場所 | 責務 |
| --- | --- |
| `include/System/Core/`, `source/System/Core/` | ウィンドウ、入力、時間、ゲームオブジェクト、Managerなど実行時の基盤 |
| `include/System/Editor/`, `source/System/Editor/` | EditorSystem、Hierarchy/Inspector、GameWindow、フィールド登録、ファイルダイアログ |
| `include/System/Serialization/`, `source/System/Serialization/` | YAMLの値変換、ドキュメント入出力、GameObjectのシリアライズ |
| `include/System/Diagnostics/`, `source/System/Diagnostics/` | Logger、LogSystem、ファイルWorker、PerformanceMonitor |
| `include/System/Rendering/`, `source/System/Rendering/` | Renderer、RendererBase、DirectXTex |
| `include/System/SystemInc.h` | 上記System公開ヘッダーをまとめて参照するインクルード窓口 |
| `include/Component/`, `source/Component/` | Transform、カメラ、メッシュ・スプライト描画、パーティクル、音声などの再利用可能な機能 |
| `include/Scene/`, `source/Scene/` | タイトル、ゲーム、リザルト各シーンの構築と振る舞い |
| `include/Script/`, `source/Script/` | プレイヤー、敵、弾、UI、演出などゲーム固有のコンポーネント |
| `include/math/`, `include/Common/` | ベクトル、曲線、色、乱数などの共通機能 |
| `Assets/Shaders/` | Git管理する`.shader`アセットとHLSLソース。コンパイル済み `.cso` は `BuildedShader/` に出力 |
| `Assets/Scenes/`、`Assets/Prefabs/`、`Assets/Materials/` | Git管理するScene、Prefab、Materialアセット |
| `Assets/Local/` | モデル、テクスチャ、サウンドなどローカル専用リソース。Git管理対象外 |
| `config/` | ImGuiテーマ、フォント、レイアウト保存先とゲーム内調整値の設定 |
| `Logs/` | 実行ごとに生成されるログファイル。Git管理対象外 |

## 実行フロー

`WinMain` は 60 FPS を目標に、`Time::Update()`、`PerformanceMonitor::RecordFrameTime()`、`Manager::Update()`、`Manager::Draw()`、`LogSystem::Flush()` を順に呼ぶ。

```text
起動
  ├─ LogSystem::Init でファイル出力Workerを開始
  └─ Manager::Init
       ├─ Renderer / Input / PostEffect / Audio を初期化
       ├─ Editor構成では ImGui / Docking / Viewport / EditorWindow群を初期化
       └─ TitleScene を予約

毎フレーム: Manager::Update
  ├─ 入力を更新
  ├─ Editor構成では Game View の要求サイズを適用し、ImGui フレームを更新
  ├─ Editor構成ではGameWindowのレイアウト確定とRenderTargetサイズ適用後に保留中Sceneを初期化
  ├─ 保留中シーンがなければ、保留中オブジェクトを有効化
  ├─ Scene::Update
  ├─ GameObject::Update
  ├─ Physicsの固定ステップ、Transform同期、接触イベント配信
  ├─ GameObject::LateUpdate
  └─ Destroy 指定済みオブジェクトを OnDestroy 後に削除

毎フレーム: Manager::Draw
  ├─ Game RenderTarget を開始し、メインカメラの View / Projection を設定
  ├─ RendererBase を描画コマンドとして送信・ソート・描画
  ├─ PostEffect を適用
  ├─ Editor構成では BackBuffer RTV に切り替えて GameWindow / Inspector / Debug を描画
  └─ Editor構成は Present、Runtime構成は Game RenderTarget を BackBuffer へ CopyResource して Present

フレーム終端: LogSystem::Flush
  ├─ 最大100件を取り出して現在秒のログへ集約
  ├─ Editor構成では現在秒の件数をImGuiへ反映
  └─ 秒が変わったら確定ログをファイルWorkerへ配送

終了
  ├─ Manager::Uninit
  └─ LogSystem::Shutdown で残件を書き出してWorkerを終了
```

## GameObject と Component

`GameObject` は通常の `Transform` または UI専用の `UITransform` を1つ持ち、追加された `Component` を `std::unique_ptr` で所有する。Transform系Componentの共通基底である `TransformBase` が親子関係を一元管理し、Transform種別ごとの座標計算は各派生Componentが担当する。コンポーネントは `m_owner` で所有 GameObject にアクセスする。`Manager` も稼働中および保留中の GameObject を `std::unique_ptr` で所有し、外部には非所有の `GameObject*` を返す。

| フック | 呼び出しタイミング |
| --- | --- |
| `OnStart()` | 所有 GameObject の開始時、または開始済み GameObject への追加時 |
| `OnActive()` | GameObject が開始時に有効、または無効から有効へ遷移した時 |
| `OnEnable()` | Component が開始時に有効、または無効から有効へ遷移した時（所有 GameObject も有効な場合） |
| `Update()` | 毎フレームの主処理 |
| `LateUpdate()` | 全 `Update()` 後の後段処理 |
| `OnHitBegin()` | Collider同士が接触を開始した固定物理ステップ後 |
| `OnHit()` | 接触開始の次以降、接触が継続している固定物理ステップ後 |
| `OnHitEnd()` | Collider同士の接触が終了した固定物理ステップ後 |
| `OnDestroy()` | オブジェクトまたはシーンの破棄時 |

`AddComponent()` は所有 GameObject が開始済みの場合だけ、追加したコンポーネントの `OnStart()` を即時実行する。開始前の GameObject ではコンポーネントを保持するだけで、GameObject の `OnStart()` が各コンポーネントを追加順に開始する。prefab は必要なコンポーネントをすべて追加してから `Manager::Instantiate()` が GameObject を開始するため、コンポーネント間の依存関係を構築時に解決できる。

GameObject の有効状態は `m_isActive`、Component の有効状態は `m_isEnabled` で管理し、どちらも既定値は `true` とする。`SetActive(false)` は対象 GameObject の `Update()`、`LateUpdate()`、所有 Renderer の描画を停止する。開始済み GameObject を `SetActive(true)` にすると、全 Component の `OnActive()` を呼び出し、そのうち有効な Component の `OnEnable()` を呼び出す。`SetEnabled(false)` は対象 Component の更新と描画を停止し、開始済みかつ所有 GameObject が有効な状態で `SetEnabled(true)` にすると `OnEnable()` を呼び出す。開始時の有効状態も同じ規則でコールバック対象となり、GameObject が無効な間に Component を有効化した場合の `OnEnable()` は GameObject の再有効化まで保留する。

物理シミュレーションはPlay中だけ実行する。`Manager`は全Componentの`Update()`後に固定時間刻みでPhysicsを更新し、Dynamic Bodyの結果をTransformへ同期してから接触イベントをGameObject経由で各Componentへ配信する。その後に`LateUpdate()`を実行する。Joltのワーカースレッドからゲーム側Componentを直接呼ばない。BodyとCollider階層、Trigger、Layer、イベントの詳細は[PHYSICS.md](PHYSICS.md)を参照する。

`Manager::Update()` は `m_GameObjects.size()` を毎回評価する添字ループで GameObject を更新する。このため、`Update()` 中に生成されたオブジェクトは同じフレームの `Update()` と `LateUpdate()` の対象になり、`LateUpdate()` 中に生成されたオブジェクトも同じフレームの `LateUpdate()` の対象になる。

## Transform Hierarchy

親子関係の正は`TransformBase`が持つ。通常のWorld Objectは`Transform`、UI Objectは`UITransform`を座標計算Componentとして持ち、共通基底の親子リストへ接続される。これによりUITransformが通常TransformのLocal TRSやWorldキャッシュを継承する必要はなく、UIの比率LayoutとWorld行列をUI側へ集約できる。

```text
Manager
  └─ GameObject（全Objectを一元所有）
       └─ TransformBase Component
            ├─ parent: TransformBase*（非所有）
            ├─ children: TransformBase*[]（非所有）
            ├─ Transform（World Object）
            └─ UITransform（UI Object）
```

TransformはLocal Position、Local Quaternion Rotation、Local Scaleを保持する。Rotationの内部表現はQuaternionだけとし、Euler角を別の状態として保持しない。World行列はLocal行列と親GameObjectのTransformのWorld行列から求める。UITransformは親GameObjectのUIRectとCanvas ScaleからLayoutを導出し、独自のUI World行列を提供する。

TransformはQuaternionを扱う`GetLocalRotation()`と`SetLocalRotation()`、Euler角を扱う`GetLocalEulerAngles()`と`SetLocalEulerAngles()`を明示して提供する。既存のEuler角APIは互換性のため後者へ転送する。InspectorはEuler角を編集値として表示し、変更時にQuaternionへ変換する。YAMLにはQuaternionを`[x, y, z, w]`として保存する。Quaternion自体のAPIと演算規約は[MATH.md](MATH.md)を参照する。

Position、Rotation、Scaleの`Get...()`と`Set...()`はすべてWorld値を扱う。Local値を扱うAPIには必ず`Local`を含め、`GetLocalPosition()`、`SetLocalPosition()`、`GetLocalRotation()`、`SetLocalRotation()`、`GetLocalScale()`、`SetLocalScale()`とする。親を持たないTransformではWorld値とLocal値は一致する。

`GameObject::SetParent()`はTransformBaseへの委譲APIであり、実際の親子リスト更新はTransformBaseが担当する。自己親子、循環参照、異なるTransform種別の親子付けを拒否し、失敗時はErrorLogを記録して状態を変更しない。`nullptr`を指定するとRootへ戻す。通常Transformの親変更時は、見た目のWorld変換を維持する`KeepWorld`を既定とし、Local変換を維持する`KeepLocal`も指定できる。UITransformのLayoutはCanvasが再計算するため、UIの配置値はUITransform側の比率Layoutを正とする。

親GameObjectを`Destroy()`すると、TransformBaseの子Transformを経由して子孫にも再帰的に破棄予約を伝播する。予約されたGameObjectは同じフレームの残りの`Update()`と`LateUpdate()`を実行せず、Managerの破棄フェーズで実際に削除する。

親に非一様Scale、子に回転がある組み合わせではshearが生じる。初期実装ではTRSへの近似分解として扱い、完全なshearの保存・復元は対象外とする。

## シーンと生成

`Scene` は `Init()` を必須とし、必要に応じて `Uninit()`、`Update()`、`Draw()` を実装する。`Manager::ChangeScene<T>()` は次のフレームの更新時にシーンを切り替える。切替時には旧シーンの `Uninit()`、全 GameObject の `OnDestroy()`、オブジェクト群とポストエフェクトの破棄が行われる。

オブジェクトは以下の経路で生成する。

- `Manager::CreateEmpty()`: 空の GameObject を生成する。通常時は稼働リストへ登録してただちに `OnStart()` を呼ぶ。
- `Manager::Instantiate()`: `.prefab`からGameObject階層、Component、Field、親子参照を復元する。引数なしでは保存済みRoot Transformを維持し、PRS指定時だけRootを上書きする。

シーンの `Init()` 中に生成された GameObject は保留リストに格納される。シーン構築の完了後、同じ生成バッチの全 GameObject を稼働リストへ移してから順番に `OnStart()` を呼ぶため、別 GameObject を検索する依存関係も開始時に解決できる。通常時に生成された GameObject は生成直後から Manager の検索対象となる。

新規Prefabの標準経路は、Hierarchyから保存した`Assets/`配下の`.prefab`と`Manager::Instantiate()`である。Prefab Archiveの保存形式と復元順序は[SERIALIZATION.md](SERIALIZATION.md)を参照する。

## 描画

描画可能なコンポーネントは `RendererBase` を継承する。生成時に `Manager` のレンダラーリストへ登録され、破棄時に解除される。`Manager::Draw()` は各レンダラーの `Submit()` を呼び、`Renderer` がコマンドをソートして描画する。

描画は `RenderPhase` とソートキーで順序を制御する。

- `Opaque`: MaterialID、MeshID、描画レイヤーでソートする。
- `Transparent`: 描画レイヤーの後、カメラ前方方向の距離を用いて奥から手前へ描画する。
- `Overlay`: 描画レイヤーを優先して前面要素を描画する。

描画後には `PostEffect` を順に適用する。Editor構成ではその後にImGuiを描画し、Runtime構成ではGame RenderTargetをBackBufferへ転送する。`Bloom` は標準で `GameScene` から追加されるポストエフェクトである。

Game描画はRenderer所有の可変サイズPing-Pong RenderTextureへ行う。Editor構成の`GameWindow`はこのSRVをImGui Imageとして表示し、表示領域に合わせて次フレームのRenderTargetサイズを要求する。`Free`は領域全体、`16:9`はレターボックス付きで表示する。3D Cameraのアスペクト比とBloomの中間RenderTextureは現在のGame RenderTargetサイズへ追従し、2Dの射影と初期UI配置も現在のサイズへ追従する。`SCREEN_WIDTH`／`SCREEN_HEIGHT`は初期値・フォールバックとしてのみ使用する。

Editor / Runtime は`GM_ENABLE_EDITOR`で分岐する。Debug構成では`GM_ENABLE_EDITOR=1`としてImGui、Docking、GameWindowを有効にし、Release構成では`GM_ENABLE_EDITOR=0`としてImGui本体・backend・Editorウィンドウ群をコンパイル対象から外す。Prefab、Scene、Component、Renderer、PostEffectなどのゲームデータと実行時処理は両構成で共通に扱う。

Material、ModelAsset、Phase別キュー、PostProcess の現行設計は [RENDERING.md](RENDERING.md) を参照する。

Scene、Prefab、MaterialのYAML保存形式と、Transform親子関係を含む復元規則は[SERIALIZATION.md](SERIALIZATION.md)を参照する。

## 診断機能

ログの記録・集約・ファイル出力、性能計測、ImGuiの`Debug`ウィンドウは診断システムとして扱う。ログの記録は任意のスレッドから行い、集約と性能計測はメインスレッド、ファイル出力は専用Workerで処理する。

各クラスの責務、ログの集約規則、スレッド境界、Performance指標の計測範囲は[DIAGNOSTICS.md](DIAGNOSTICS.md)を参照する。

実行中のGameObject、Component、Materialを調整するInspector、フィールド登録、Docking、分離Viewport、Game View、テーマ設定を含むEditor全体の構成は[EDITOR.md](EDITOR.md)を参照する。

16:9基準のCanvas、UITransform、UIImage、UIText、FontAssetの設計は[UI.md](UI.md)を参照する。

Edit状態とPlay状態の分離、ライフサイクルの設計、将来のScript DLLホットリロードに向けた境界は[PLAY_MODE.md](PLAY_MODE.md)を参照する。

ゲーム固有のScript実装、Inspector調整、Scene／Prefab保存を使った標準的な開発手順は[GAME_DEVELOPMENT.md](GAME_DEVELOPMENT.md)を参照する。

現在の機能完了条件には含めない拡張・最適化項目は[FUTURE_WORK.md](FUTURE_WORK.md)を参照する。

## リソースとシェーダー

実行時リソースはプロセスの作業ディレクトリからの相対パスで指定する。Scene、Prefab、Material、Shader、Textureなどのアセットは`Assets/`配下であれば配置でき、種類別フォルダは推奨配置およびファイルダイアログの初期位置として扱う。ローカル専用リソースは`Assets/Local/`、Shader生成物は`BuildedShader/`、設定は`config/`に置く。

HLSL は Shader Model 5.0 を対象とし、`common.hlsl` は共有定義で単体コンパイルしない。各実行用シェーダーはプロジェクト設定で `.cso` へコンパイルされ、C++ 側はそのファイルをロードする。

Material描画システムでは、Material Properties用の定数バッファは `b3`、Material Propertiesから宣言する `Texture2D` は `t16` 以降を使用する。Propertiesのメモリ配置はC++構造体のサイズではなく、コンパイル後のHLSLリフレクションが返す変数のoffsetとsizeを正とする。
