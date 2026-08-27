# Edit / Play モード設計

## 目的

Editor上の編集状態（Edit）と、ゲーム実行中の状態（Play）を分離する。Edit中にSceneやPrefabの構成、Inspectorで設定した値だけを静的データとして扱い、Play中に生成・破棄・移動したGameObjectや変化したFieldが編集データやSceneファイルへ混入しないようにする。

本資料は現在実装済みの基本運用と、今後の拡張方針を扱う。実装済み範囲はSceneの自動保存・再読込によるEdit / Play分離、Scene選択、必要時のEditor再評価である。

## モード

| モード | 目的 | 許可する処理 |
| --- | --- | --- |
| Edit | Scene / Prefabの編集とプレビュー | Inspector編集、静的なリソース解決、描画、明示的な保存 |
| Play | 編集状態を元にしたゲーム実行 | ライフサイクル、更新、生成・破棄、シーン遷移、音声再生 |

`Manager`が`GameMode`（`Edit` / `Play`）を所有し、EditorのPlayボタンで遷移する。Runtime構成は起動時からPlayとして動作する。

## EditからPlayへの遷移

Play開始時に、現在のEdit状態を現在選択中のSceneファイルへ自動保存する。続いて全GameObjectの`OnStart()`を呼び、Play中の更新を有効にする。Stopは描画中にオブジェクトを破棄しないよう次の`Manager::Update()`冒頭で処理し、同じSceneを再初期化して自動保存した内容へ戻す。これにより、Play開始前のInspector編集は保持しつつ、Edit用とPlay用のObject群を常時二重化する必要がない。

```text
Edit
  ├─ 現在のSceneファイルへ自動保存
  ├─ 全Objectを開始
  └─ Play

Play
  ├─ Script / Component / Sceneを更新
  ├─ 動的な生成・破棄を許可
  └─ Stop

Stop
  ├─ Play用状態を破棄
  ├─ 自動保存したSceneファイルからEdit状態を復元
  └─ Edit
```

Play中の`SaveCurrentScene()`、Scene保存、Prefab保存は無効化する。Play中の値を明示的に編集データへ反映する機能は初期対象外とする。

## Scene選択

Editorのメニューバーには`Open Scene`を設ける。ファイルダイアログで`Assets/Scenes`配下の`.scene`を選択し、Editモードで読み込む。選択したパスを`Manager`の現在Sceneとして保持し、Play開始時の自動保存とStop時の再読込に使用する。

Play中はScene選択を無効化する。Sceneを切り替える場合はStopしてから選択する。

## Componentライフサイクル

現在の`OnStart()`には、リソース解決とゲーム開始時の副作用が混在している。Editでは前者が必要であり、後者は実行してはならない。そのため、以下の責務へ分離する。

| フック | 呼び出し時期 | 許可する処理 |
| --- | --- | --- |
| `OnLoaded()` | YAMLのField、親子、参照の復元完了後。Edit / Play共通 | パスからのリソース解決、GPUバッファ準備、同一Object内Componentの参照解決 |
| `OnEditorUpdate()` | Edit中に再評価が必要になった時だけ | Inspector編集後などに行うComponentごとのプレビュー・補助処理 |
| `OnStart()` | Play開始時のみ | GameObject生成、音声の自動再生、ゲーム開始処理 |
| `OnActive()` / `OnEnable()` | Play中の有効化時のみ | 実行時の再開処理 |
| `Update()` / `LateUpdate()` | Play中のみ | フレームごとのゲーム処理 |
| `OnDestroy()` | Play状態の破棄時、またはPlay中の破棄時 | 実行時リソースと参照の解放 |

`OnLoaded()`は再実行可能とし、GameObjectの生成・破棄、シーン遷移、再生開始などの状態変化を行わない。

`Component`は`OnLoaded()`と`OnEditorUpdate()`を仮想関数として宣言する。`Script`は両方を空実装で`override final`し、Script派生型から再overrideできないようにする。これにより、標準Componentだけがリソース解決やEditor用再評価を実装し、基底Componentの実装がScript派生型へ伝播しない。

既存Componentは次のように整理する。

- `ModelRenderer`、`SpriteRenderer`、`TrailRenderer`、`ParticleRenderer`: Material / Modelの解決とGPUリソース準備を`OnLoaded()`へ移す。
- `Audio`: 音声データのロードを`OnLoaded()`、`PlayOnStart`による再生を`OnStart()`へ残す。
- `Skydome`: 子GameObjectを生成するため`OnStart()`のままとする。
- `Script`派生: 原則としてゲーム固有の副作用を`OnStart()`以降へ置く。

Inspectorでパスを編集した直後など、再解決が必要な時は対象Componentの`OnEditorUpdate()`を明示的に呼ぶ。`OnEditorUpdate()`を毎フレーム呼ばず、Field変更・選択変更など呼び出し元が必要性を判断できる契機に限定する。

## 更新と保存の規則

Edit中は`Scene::Update()`、`GameObject::Update()`、`GameObject::LateUpdate()`を呼ばない。毎フレームの代替更新も行わない。Editor側が再評価を要求した時だけ、対象Componentの`OnEditorUpdate()`を呼ぶ。これにより、通常の更新処理は静的編集状態を変更せず、Editor用の振る舞いだけを明示的に実装できる。

`Manager::CreateEmpty()`と`Instantiate()`は、生成直後に`OnLoaded()`を呼ぶ。`OnStart()`はPlay中だけ呼ぶ。Edit中に追加したComponentはPlay開始時に、Play中に追加したComponentは追加時に開始する。

## 将来のScript DLLホットリロード

ScriptをDLLへ分離してホットリロードする構想は、Edit / Play分離の後に扱う。現状はScriptが`GameObject`、`Manager`、標準Componentを直接参照し、`ComponentRegistry`が`typeid`と`std::function`のFactoryを保持する。そのままDLLをunloadすると、RegistryのFactoryやScriptの仮想関数テーブルが解放済みコードを参照する。

ホットリロード対応時は、次の境界を設ける。

- Engine: Core、Serializer、Renderer、Editor、標準Componentを所有する。
- GameScript DLL: `Script`派生型だけを実装する。
- 登録: DLLは`extern "C"`の登録・解除APIで、文字列型名と生成・破棄関数をEngineへ渡す。
- unload: Play停止後にScriptインスタンスを破棄し、Registryから登録を解除してからDLLをunloadする。
- reload: DLLを読み直して再登録し、保存済みFieldからScriptを再生成する。

将来のDLL境界では、`typeid`をモジュール間の識別子に使わず、YAMLと共通の文字列型名を正とする。C++ ABI、RTTI、`std::function`をDLL境界へ露出しない。

## 実装状況と次の段階

- [x] `GameMode`とGameWindowのPlay / Stop操作、Scene選択ダイアログを追加する。
- [x] Play開始時のScene自動保存とStop時の再読込を実装する。
- [x] Edit中に更新・開始コールバックを実行しないよう`Manager`と`GameObject`の開始処理を分離する。
- [x] YAML復元完了後の`OnLoaded()`を追加する。
- [x] `OnEditorUpdate()`を追加し、InspectorのField変更時に対象Componentだけを呼ぶ。
- [x] Play中のScene / Prefab保存を無効化する。
- [x] Edit / Playの生成、破棄、有効化、保存、Stop復元を実機確認する。
- [ ] 描画・音声Componentのリソース解決を必要に応じて`OnLoaded()`へ移し切る。
- [ ] 上記が安定した後、Script DLLの登録・解除APIを設計する。
