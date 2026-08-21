# アーキテクチャ

## 概要

Project_Drive は、Windows / DirectX 11 を基盤とするコンポーネント指向のゲームフレームワークである。アプリケーションループは `source/System/main.cpp` にあり、`Manager` がシーン、ゲームオブジェクト、描画、ポストエフェクトの進行を統括する。

```text
WinMain
  ├─ Manager
  │    ├─ Scene
  │    │    └─ GameObject
  │    │         └─ Component
  │    ├─ Renderer / RendererBase
  │    ├─ PostEffect
  │    └─ Prefab factory
  └─ 診断システム
       ├─ Logger / LogSystem / LogFileWorker
       ├─ PerformanceMonitor
       └─ DebugWindow
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
| `Logs/` | 実行ごとに生成されるログファイル。Git管理対象外 |

## 実行フロー

`WinMain` は 60 FPS を目標に、`Time::Update()`、`PerformanceMonitor::RecordFrameTime()`、`Manager::Update()`、`Manager::Draw()`、`LogSystem::Flush()` を順に呼ぶ。

```text
起動
  ├─ LogSystem::Init でファイル出力Workerを開始
  └─ Manager::Init
       ├─ Renderer / Input / PostEffect / Audio / ImGui を初期化
       └─ TitleScene を初期化

毎フレーム: Manager::Update
  ├─ 入力と ImGui フレームを更新
  ├─ 保留中シーンがあれば切替、なければ保留中オブジェクトを有効化
  ├─ Scene::Update
  ├─ GameObject::Update
  ├─ GameObject::LateUpdate
  └─ Destroy 指定済みオブジェクトを OnDestroy 後に削除

毎フレーム: Manager::Draw
  ├─ レンダーターゲットを開始し、メインカメラの View / Projection を設定
  ├─ RendererBase を描画コマンドとして送信・ソート・描画
  ├─ PostEffect を適用
  ├─ ImGui を描画
  └─ 表示

フレーム終端: LogSystem::Flush
  ├─ 最大100件を取り出して現在秒のログへ集約
  ├─ 現在秒の件数をImGuiへ反映
  └─ 秒が変わったら確定ログをファイルWorkerへ配送

終了
  ├─ Manager::Uninit
  └─ LogSystem::Shutdown で残件を書き出してWorkerを終了
```

## GameObject と Component

`GameObject` は `Transform` を必ず持ち、追加された `Component` を `std::unique_ptr` で所有する。コンポーネントは `m_owner` で所有 GameObject にアクセスする。`Manager` も稼働中および保留中の GameObject を `std::unique_ptr` で所有し、外部には非所有の `GameObject*` を返す。

| フック | 呼び出しタイミング |
| --- | --- |
| `OnStart()` | 所有 GameObject の開始時、または開始済み GameObject への追加時 |
| `Update()` | 毎フレームの主処理 |
| `LateUpdate()` | 全 `Update()` 後の後段処理 |
| `OnDestroy()` | オブジェクトまたはシーンの破棄時 |

`AddComponent()` は所有 GameObject が開始済みの場合だけ、追加したコンポーネントの `OnStart()` を即時実行する。開始前の GameObject ではコンポーネントを保持するだけで、GameObject の `OnStart()` が各コンポーネントを追加順に開始する。prefab は必要なコンポーネントをすべて追加してから `Manager::Instantiate()` が GameObject を開始するため、コンポーネント間の依存関係を構築時に解決できる。

`Manager::Update()` は `m_GameObjects.size()` を毎回評価する添字ループで GameObject を更新する。このため、`Update()` 中に生成されたオブジェクトは同じフレームの `Update()` と `LateUpdate()` の対象になり、`LateUpdate()` 中に生成されたオブジェクトも同じフレームの `LateUpdate()` の対象になる。

## シーンと生成

`Scene` は `Init()` を必須とし、必要に応じて `Uninit()`、`Update()`、`Draw()` を実装する。`Manager::ChangeScene<T>()` は次のフレームの更新時にシーンを切り替える。切替時には旧シーンの `Uninit()`、全 GameObject の `OnDestroy()`、オブジェクト群とポストエフェクトの破棄が行われる。

オブジェクトは以下の経路で生成する。

- `Manager::CreateEmpty()`: 空の GameObject を生成する。通常時は稼働リストへ登録してただちに `OnStart()` を呼ぶ。
- `Manager::Instantiate()`: 登録済み prefab を構築する。通常時は稼働リストへ登録してただちに `OnStart()` を呼ぶ。

シーンの `Init()` 中に生成された GameObject は保留リストに格納される。シーン構築の完了後、同じ生成バッチの全 GameObject を稼働リストへ移してから順番に `OnStart()` を呼ぶため、別 GameObject を検索する依存関係も開始時に解決できる。通常時に生成された GameObject は生成直後から Manager の検索対象となる。

prefab の実装は `include/System/PrefabFactory.h` にあり、各ファクトリは構築済み GameObject を `std::unique_ptr` で返す。`Manager::Init()` で名前とファクトリを登録し、`Manager::Instantiate()` が返された所有権を受け取る。新しい prefab はファクトリの実装と登録の両方を追加して利用する。

## 描画

描画可能なコンポーネントは `RendererBase` を継承する。生成時に `Manager` のレンダラーリストへ登録され、破棄時に解除される。`Manager::Draw()` は各レンダラーの `Submit()` を呼び、`Renderer` がコマンドをソートして描画する。

描画は `RenderPhase` とソートキーで順序を制御する。

- `Opaque`: MaterialID、MeshID、描画レイヤーでソートする。
- `Transparent`: 描画レイヤーの後、カメラ前方方向の距離を用いて奥から手前へ描画する。
- `Overlay`: 描画レイヤーを優先して前面要素を描画する。

描画後には `PostEffect` を順に適用し、最後に ImGui を描画する。`Bloom` は標準で `GameScene` から追加されるポストエフェクトである。

Material、ModelAsset、Phase別キュー、PostProcess の現行設計は [RENDERING.md](RENDERING.md) を参照する。

## 診断機能

ログの記録・集約・ファイル出力、性能計測、ImGuiの`Debug`ウィンドウは診断システムとして扱う。ログの記録は任意のスレッドから行い、集約と性能計測はメインスレッド、ファイル出力は専用Workerで処理する。

各クラスの責務、ログの集約規則、スレッド境界、Performance指標の計測範囲は[DIAGNOSTICS.md](DIAGNOSTICS.md)を参照する。

## リソースとシェーダー

実行時リソースはプロセスの作業ディレクトリからの相対パスで指定する。主な配置は `Assets\\Models`、`Assets\\Textures`、`Assets\\Sounds`、`BuildedShader`、`config` である。

HLSL は Shader Model 5.0 を対象とし、`common.hlsl` は共有定義で単体コンパイルしない。各実行用シェーダーはプロジェクト設定で `.cso` へコンパイルされ、C++ 側はそのファイルをロードする。

Material描画システムでは、Material Properties用の定数バッファは `b3`、Material Propertiesから宣言する `Texture2D` は `t16` 以降を使用する。Propertiesのメモリ配置はC++構造体のサイズではなく、コンパイル後のHLSLリフレクションが返す変数のoffsetとsizeを正とする。
