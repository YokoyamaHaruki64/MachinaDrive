# アーキテクチャ

## 概要

Project_Drive は、Windows / DirectX 11 を基盤とするコンポーネント指向のゲームフレームワークである。アプリケーションループは `source/System/main.cpp` にあり、`Manager` がシーン、ゲームオブジェクト、描画、ポストエフェクトの進行を統括する。

```text
WinMain
  └─ Manager
       ├─ Scene
       │    └─ GameObject
       │         └─ Component
       ├─ Renderer / RendererBase
       ├─ PostEffect
       └─ Prefab factory
```

## モジュール構成

| 場所 | 責務 |
| --- | --- |
| `include/System/`, `source/System/` | ウィンドウ、入力、時間、描画、ゲームオブジェクトとシーンの管理 |
| `include/Component/`, `source/Component/` | Transform、カメラ、メッシュ・スプライト描画、パーティクル、音声などの再利用可能な機能 |
| `include/Scene/`, `source/Scene/` | タイトル、ゲーム、リザルト各シーンの構築と振る舞い |
| `include/Script/`, `source/Script/` | プレイヤー、敵、弾、UI、演出などゲーム固有のコンポーネント |
| `include/math/`, `include/Common/` | ベクトル、曲線、色、乱数などの共通機能 |
| `shader/` | HLSL ソース。コンパイル済み `.cso` は `BuildedShader/` に出力 |
| `Assets/` | 実行時に読むモデル、テクスチャ、サウンド |
| `config/` | ImGui とゲーム内調整値の設定 |

## 実行フロー

`WinMain` は 60 FPS を目標に、`Time::Update()`、`Manager::Update()`、`Manager::Draw()` を順に呼ぶ。

```text
起動: Manager::Init
  ├─ Renderer / Input / PostEffect / Audio / ImGui を初期化
  └─ TitleScene を初期化

毎フレーム: Manager::Update
  ├─ 入力と ImGui フレームを更新
  ├─ 保留中オブジェクトを有効化
  ├─ 保留中シーンがあれば切替
  ├─ GameObject::Update
  ├─ GameObject::LateUpdate
  └─ Destroy 指定済みオブジェクトを OnDestroy 後に削除

毎フレーム: Manager::Draw
  ├─ レンダーターゲットを開始し、メインカメラの View / Projection を設定
  ├─ RendererBase を描画コマンドとして送信・ソート・描画
  ├─ PostEffect を適用
  ├─ ImGui を描画
  └─ 表示
```

## GameObject と Component

`GameObject` は `Transform` を必ず持ち、追加された `Component` を `std::unique_ptr` で所有する。コンポーネントは `m_owner` で所有 GameObject にアクセスする。

| フック | 呼び出しタイミング |
| --- | --- |
| `OnStart()` | シーン初期化完了時、または `AddComponent()` による即時追加時 |
| `Update()` | 毎フレームの主処理 |
| `LateUpdate()` | 全 `Update()` 後の後段処理 |
| `OnDestroy()` | オブジェクトまたはシーンの破棄時 |

`AddComponent()` は追加直後に `OnStart()` を実行する。一方、シーンや prefab の構築中は `AddComponentForCreate()` で追加し、`Manager` がシーン初期化を完了した後に一括で `OnStart()` を実行する。依存するコンポーネントがある場合、この初期化順序を前提にする。

## シーンと生成

`Scene` は `Init()` を必須とし、必要に応じて `Uninit()`、`Update()`、`Draw()` を実装する。`Manager::ChangeScene<T>()` は次のフレームの更新時にシーンを切り替える。切替時には旧シーンの `Uninit()`、全 GameObject の `OnDestroy()`、オブジェクト群とポストエフェクトの破棄が行われる。

オブジェクトは以下の経路で生成する。

- `Manager::CreateEmpty()`: 空の GameObject を保留リストに追加する。
- `Manager::InstantiateForBeginScene()`: prefab を生成し、シーン初期化完了時に `OnStart()` を呼ぶ。
- `Manager::Instantiate()`: prefab を生成し、ただちに `OnStart()` を呼ぶ。

prefab の実装は `include/System/PrefabFactory.h` にあり、`Manager::Init()` で名前とファクトリを登録する。新しい prefab はこの両方を追加して利用する。

## 描画

描画可能なコンポーネントは `RendererBase` を継承する。生成時に `Manager` のレンダラーリストへ登録され、破棄時に解除される。`Manager::Draw()` は各レンダラーの `Submit()` を呼び、`Renderer` がコマンドをソートして描画する。

描画は `RenderPass` とソートキーで順序を制御する。

- `Opaque`: 描画レイヤー、ブレンド、カリングでソートする。
- `Transparent`: 描画レイヤーの後、カメラ前方方向の距離を用いて奥から手前へ描画する。
- `Overlay`: 描画レイヤーを用いて前面要素を描画する。

描画後には `PostEffect` を順に適用し、最後に ImGui を描画する。`Bloom` は標準で `GameScene` から追加されるポストエフェクトである。

Material描画へ移行する際のPhase別キュー、DrawItem、ソート規約は [MATERIAL_RENDERING_DESIGN.md](MATERIAL_RENDERING_DESIGN.md) を参照する。

## リソースとシェーダー

実行時リソースはプロセスの作業ディレクトリからの相対パスで指定する。主な配置は `Assets\\Models`、`Assets\\Textures`、`Assets\\Sounds`、`BuildedShader`、`config` である。

HLSL は Shader Model 5.0 を対象とし、`common.hlsl` は共有定義で単体コンパイルしない。各実行用シェーダーはプロジェクト設定で `.cso` へコンパイルされ、C++ 側はそのファイルをロードする。

Material描画システムでは、Material Properties用の定数バッファは `b3`、Material Propertiesから宣言する `Texture2D` は `t16` 以降を使用する。Propertiesのメモリ配置はC++構造体のサイズではなく、コンパイル後のHLSLリフレクションが返す変数のoffsetとsizeを正とする。
