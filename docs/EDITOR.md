# Editor UI

## 目的

Debug構成で、Scene・Prefab・Materialを編集しながらGame描画を確認する。Editorはゲーム実行機能と同じScene、Component、Rendererを利用し、ImGuiは編集用UIとしてのみ追加する。

Release構成は`GM_ENABLE_EDITOR=0`でビルドする。ImGui本体、Win32 / DX11 backend、EditorWindow群はコンパイル対象から除外され、Game RenderTargetをBackBufferへ転送して表示する。

## ウィンドウ構成

`Manager::Update()`はImGuiフレーム開始直後に`DockSpaceOverViewport()`を作成する。すべてのEditorWindowは`EditorWindow`基底を継承し、初回表示サイズ、Windowサイズ、Contentサイズを共通管理する。

```text
メニューバー
  ├─ Edit
  │   ├─ Undo
  │   └─ Redo
  ├─ Settings
  │   └─ Style Reload
  └─ Open Scene（Edit中のみ）

DockSpace
  ├─ Game
  ├─ Hierarchy
  ├─ Inspector
  ├─ Performance
  └─ Log
```

Dockingを有効にしているため、各WindowはDockSpace内で再配置・タブ化できる。Viewportsも有効であり、DockSpaceから外へドラッグしたWindowはOSの別Windowとして表示できる。

`config/imgui.ini`にはDocking配置とTreeの開閉状態を保存する。開発者ごとの差分になるためGit管理しない。ComponentとMaterial SlotのTreeは初回だけ開き、その後はこのファイルに保存した状態を使う。

## Game Window

GameWindowは`Renderer`が所有するGame RenderTargetのSRVをImGui Imageとして表示する。WindowのContent領域を次フレームのRenderTargetサイズとして要求し、RendererはPing-Pong RenderTextureとDepthStencilを同じサイズで再生成する。

- Edit中は`Free`モードでContent領域をそのまま表示する。
- Play中は`16:9`に固定し、余白をレターボックスとして残す。
- `Play`は現在Sceneを自動保存してPlayへ遷移し、`Stop`は自動保存したSceneを再読込してEditへ戻す。詳細は[PLAY_MODE.md](PLAY_MODE.md)を参照する。

初期フレームに未確定のサイズでSceneを初期化しないよう、Editor構成ではGameWindowのレイアウトとRenderTargetサイズが一致してから最初のSceneを初期化する。

## HierarchyとInspector

Hierarchyは稼働中のGameObjectをTransformの親子関係に沿って再帰表示する。上部には`Save Scene`と現在のScene名を表示し、Edit中は`Save Scene`で現在Sceneを保存できる。GameObjectのドラッグ＆ドロップで親を変更し、末尾のRoot領域へドロップすると親を解除する。右クリックメニューからEmpty生成、GameObject削除、Prefab保存、PrefabのInstantiateを行う。

Inspectorでは選択中GameObjectの名前、Active、開始状態、破棄予約状態、Transform、追加順のComponentを編集する。`CreateEmpty()`の既定名は`Empty`、PrefabのInstantiate後のRoot名は`<Prefab名>(Instance)`とする。各Componentのヘッダーには`Enabled`チェックを表示し、有効化時は通常のライフサイクル規則に従って`OnActive()`または`OnEnable()`が呼ばれる。TransformはWorld／Localを切り替えてPosition、Rotation、Scaleを編集できる。

ComponentとMaterial SlotのTreeは初回表示時に開いた状態とし、その後の開閉状態はImGuiレイアウトファイルへ保存する。Fieldを編集した時だけ対象Componentの`OnEditorUpdate()`を呼び、毎フレームのEditor更新は行わない。

## 編集状態、履歴、選択

### 未保存変更

Editorは現在のSceneに未保存変更があるかを保持する。Inspectorでの値・名前・Active / Enabledの変更、Transform親子関係の変更、GameObject / Componentの生成・削除をScene変更として扱う。Sceneを切り替える`Open Scene`とアプリ終了時に未保存変更があれば、保存して続行、保存せず続行、キャンセルを選べる確認ダイアログを表示する。

Play開始時は確認を表示しない。現在の規則どおりSceneを自動保存してPlayへ移行し、Stop時にはその保存内容からEdit状態を復元する。MaterialはSceneとは独立したアセットのため、未保存状態もSceneとは分けて管理する。

### Undo / Redo

Editor操作は`EditorCommand`の`Execute()`と`Undo()`で表し、EditorSystemがUndo / Redoスタックを所有する。新しいコマンドを実行した時はRedoスタックを破棄する。`Ctrl+Z`でUndo、`Ctrl+Y`でRedoを実行し、メニューバーの`Edit`にも同じ操作を表示する。

数値入力やドラッグは編集開始時の値と編集完了時の値を1つのコマンドとして記録する。これにより、1回のスライダー操作で履歴が多数作られることを避ける。値変更は汎用コマンドで扱えるが、GameObject / Componentの生成・削除、親子変更は復元に必要な情報を保持する専用コマンドとする。

Undo / Redoの対象は、Inspectorの値、名前、Active / Enabled、Transform、親子変更、GameObject / Componentの生成・削除とする。コマンド実行、Undo、RedoのいずれもSceneをdirtyにし、必要なComponentだけ`OnEditorUpdate()`を呼ぶ。

### Hierarchyの複数選択

選択状態は単一の`GameObject*`ではなく、選択順を保持するGameObject配列と範囲選択の起点で管理する。

- 通常クリックは単一選択へ置き換える。
- `Ctrl`クリックは対象を選択へ追加または解除する。
- `Shift`クリックは起点からクリック位置まで、Hierarchyに表示されている行を範囲選択する。

初期段階では複数選択中のInspectorは選択数だけを表示し、複数Objectへの一括Field編集は扱わない。複数選択での生成・削除などの操作は、Undo / Redoコマンドとしてまとめて扱う。

## Field登録

Componentは個別のImGui実装ではなく、`RegisterFields()`で調整対象を登録する。登録済みFieldはInspector表示とYAMLのSerialize／Deserializeで共用し、編集UIと保存対象の重複を避ける。

```cpp
class Component
{
public:
    bool DrawInspectorImGui(InspectorFieldSpace inspectorSpace = InspectorFieldSpace::All);
    bool ToYAMLValue(YAMLValue& output, const YAMLValueConversionContext& context = {});
    bool FromYAMLValue(const YAMLValue& input, const YAMLValueConversionContext& context = {});

protected:
    virtual void RegisterFields(FieldRegistry& fields) {}
};
```

新規のComponent／Scriptでは、`System/GameplayMacros.h`のマクロで宣言と定義の定型を省略できる。`USE_FIELDS`は`RegisterFields()`の宣言、`FIELDS_BEGIN(Type)`から`FIELDS_END()`はその定義へ展開する。

```cpp
// Header
class Player : public Script
{
    USE_FIELDS
};

// Source
FIELDS_BEGIN(Player)
    GM_FIELD(m_moveSpeed);
    GM_FIELD(m_jumpPower);
FIELDS_END()
```

`GM_FIELD(member)`はメンバー名を保存キーとして使用し、表示名は`m_moveSpeed`から`Move Speed`のように整形する。保存データとの互換性を保つ必要がある場合は、明示キー版の`GM_FIELD_KEY("moveSpeed", m_moveSpeed)`を使う。Getter／Setterを通す必要がある値はAccessor系マクロで登録する。

`Reflect<T>::Members()`を持つ独自型も`GM_FIELD`で登録できる。InspectorはReflectのtupleを再帰走査し、YAMLには親キー配下のMapとして保存する。World／Localの切り替え、Accessor、Hiddenなど個別の編集規則が必要な値は、Accessor登録または個別描画を使う。

### 参照とパス

`GameObjectReference`は稼働中GameObjectのコンボボックスから選択する。`PrefabReference`は`.prefab`用のファイルダイアログから選択する。

ファイルパスを扱う`std::string`は通常の文字入力にせず、`GM_FIELD_PATH`で登録する。Inspectorにはプロジェクト相対パスと`Select`／`Clear`を表示し、`Assets/`配下のファイルだけを拡張子フィルター付きダイアログから選択する。

```cpp
GM_FIELD_PATH(m_filePath, "Assets/Local/Sounds", ".wav");
GM_FIELD_PATH(m_modelPath, "Assets/Local/Models", ".obj", ".fbx", ".gltf");
```

### Field Type

`InspectorFieldType::Auto`はC++の型から表示方法を選択する。明示指定が必要な場合だけField Typeを与える。

| Field Type | 用途 |
| --- | --- |
| `Auto` | 基本型から自動選択する既定値 |
| `Slider` | 範囲を持つ数値のスライダー表示 |
| `List` | 配列・コンテナの一覧表示 |
| `Curve` | Curve型専用の編集UI |
| `FilePath` | `Assets/`配下のファイルをダイアログから選択 |
| `Hidden` | YAML対象として登録するがInspectorには表示しない値 |

`Auto`では`bool`、`int`、`float`、`std::string`、各Vector、Color、Matrix、Quaternion、Curve、GameObjectReference、PrefabReferenceを扱う。ButtonなどのActionはFieldにも保存対象にも含めず、必要になった時点で専用の操作APIとして追加する。

## Material Inspector

Renderer ComponentのInspectorは、固定Material SlotごとにMaterialを表示する。`Select Material`で`Assets/`配下の`.material`を割り当て、Material未設定または参照エラー時は`Create Material`からShaderを選択して未保存Materialを作成する。

有効なMaterialは名前、Shader、Property、保存先を表示する。`Select Shader`はShaderパスをダイアログから選び、現在のMaterialへ適用する。未保存Materialには`Save Material As`、保存済みMaterialには`Save Material`を表示する。

Material Propertyの型別描画はMaterial側へ集約し、Renderer Component側は保持Materialを選んで表示する。SpriteRendererなどが毎フレーム供給するPropertyは、Inspectorで変更しても次の供給時に上書きされる。

## 編集上の制約

- Fieldは値の編集だけを扱い、GameObject生成・破棄・親変更、Component追加は専用操作として分離する。
- ComponentごとのImGui IDスコープはInspector側で作成し、同名Fieldを持つ複数Componentの衝突を防ぐ。
- `RegisterFields()`はInspector・保存・読込のたびに再実行し、一時的な参照を永続保持しない。

## テーマとフォント

`config/imgui_style.toml`はEditorのテーマ、フォント、ImGuiレイアウト保存先をまとめる。設定は標準のDarkテーマへ上書き適用するため、未指定の項目はImGuiの既定値を使う。

| キー | 内容 |
| --- | --- |
| `FontPath` | 日本語グリフを含むTrueTypeフォントのパス。起動時に読み込む。 |
| `IniPath` | ImGuiレイアウト保存先。 |
| ImGui色名 | `#RRGGBB`または`#RRGGBBAA`で色を指定する。 |
| 数値項目 | 角丸、枠線、余白を指定する。 |

`Settings > Style Reload`は同じTOMLを再読込して色・数値設定と`IniPath`を再適用する。同じキーを複数回記述した場合は記述順に適用し、最後の値が有効になる。フォントアトラスは起動時に生成するため、`FontPath`の変更は再起動後に反映する。

TOMLは小規模な設定用の単純な`キー = 値`形式として読み込む。コメントは`#`、文字列はダブルクォートで記述する。現在対応する数値設定は`WindowRounding`、`FrameRounding`、`PopupRounding`、`ScrollbarRounding`、`TabRounding`、Tab各種Border / Overline、Window / Frame Border、`ItemSpacing`、`WindowPadding`である。
