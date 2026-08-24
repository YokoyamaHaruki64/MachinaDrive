# 実行時 Inspector 設計

## 目的

実行中の GameObject、Component、Material を ImGui から確認・調整する。Component が宣言する調整フィールドは Inspector とシリアライズで共用し、個別の ImGui 実装と保存対象の重複を避ける。

## UI 構成

Inspector は `Debug` ウィンドウ内ではなく、独立した次の二つの ImGui ウィンドウとして表示する。

```text
Hierarchy Window
  └─ 稼働中 GameObject の一覧
       └─ 選択

Inspector Window
  └─ 選択中 GameObject
       ├─ GameObject 共通情報
       ├─ Transform
       └─ Component ごとの Inspector 項目
            └─ Renderer の固定 Material Slot
                 └─ Material Property
```

`Hierarchy Window` は稼働中の GameObject を名前で表示する。選択状態は非所有ポインタとして保持し、シーン遷移・破棄などで選択先が稼働リストから消えた場合は解除する。

`Inspector Window` は選択中 GameObject の名前、開始状態、破棄予約状態、Transform、追加順の Component を表示する。Transform も通常の Component と同じフィールド登録方式を使い、Position、Rotation、Scale を編集できる。

HierarchyはTransformの親子関係を再帰表示する。GameObjectのドラッグ＆ドロップで親を変更し、末尾のRoot領域へドロップすると親を解除する。右クリックメニューからEmpty生成、GameObject削除、Prefab保存、Prefab Instantiateを実行できる。Inspectorの`Add Component`はComponent Registryに登録された型をComponent／Scriptに分類して表示する。

Inspector は `Manager::Draw()` の ImGui 描画区間で実行する。このため、Inspector で変更した Transform や Material Property が描画へ反映されるのは原則として次フレームとなる。

## GameObject 名

GameObject は `std::string` の名前を保持し、`GetName()` と `SetName()` を提供する。

- `Instantiate(prefabName, ...)` が生成する GameObject の既定名は `prefabName(Instance)` とする。
- `CreateEmpty(...)` の既定名は `Empty` とする。
- `CreateEmpty(name, ...)` の名前指定オーバーロードを追加する。
- 既存の `CreateEmpty(position, rotation, scale)` は維持し、呼び出し側の互換性を保つ。

## Component フィールド登録

各 Component は個別の ImGui 描画関数ではなく、フィールド登録フックを実装する。登録したフィールドは Inspector 表示と Serialize / Deserialize の共通対象である。

```cpp
class Component
{
public:
    void DrawInspectorImGui();
    bool EncodeFields(YAMLValue& output, const YAMLArchiveContext& context);
    bool DecodeFields(const YAMLValue& input, const YAMLArchiveContext& context);

protected:
    virtual void RegisterFields(FieldRegistry& fields) {}
};
```

Inspector描画とYAMLのEncode／Decodeは基底Componentが管理し、それぞれ`FieldRegistry`を作成して`RegisterFields()`を呼ぶ。派生Componentはこの共通経路を上書きしない。

利用側は、基本的に一変数につき一行で登録する。

```cpp
void Player::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_moveSpeed);
    GM_FIELD(m_jumpPower);
    GM_FIELD(m_isGrounded);
    GM_FIELD(m_spawnPosition);
}
```

`GM_FIELD(member)` はメンバー名を保存キーとして文字列化し、参照型から型情報を自動判定して `FieldRegistry` へ登録する。表示名は `m_moveSpeed` のようなメンバー名から `Move Speed` へ整形する。

メンバー名の変更後も保存データとの互換性を保つ必要があるフィールドには、明示キー版を使う。

```cpp
GM_FIELD_KEY("moveSpeed", m_moveSpeed);
```

getter／setterを通す必要がある値も、一行のAccessorマクロで登録できる。Transformはこの経路でWorld／Local値の編集、角度変換、dirty伝播を維持する。

```cpp
GM_FIELD_ACCESSOR(Vector3, "position", GetPosition(), SetPosition(value), World);
GM_FIELD_ACCESSOR_TYPE(
    Quaternion,
    "m_localRotation",
    GetLocalRotation(),
    SetLocalRotation(value),
    All,
    InspectorFieldType::Hidden,
    true);
```

## Field Type と Inspector 表現

`InspectorFieldType::Auto` は C++ の型から Inspector 表現を選択する。明示指定が必要な場合だけ Field Type を与える。

```cpp
GM_FIELD(m_moveSpeed);
GM_FIELD_TYPE(m_hitPoint, InspectorFieldType::Slider);
GM_FIELD_TYPE(m_patrolPoints, InspectorFieldType::List);
GM_FIELD_TYPE(m_motionCurve, InspectorFieldType::Curve);
```

初期の Field Type は次の通りとする。

| Field Type | 用途 |
| --- | --- |
| `Auto` | 基本型から自動選択する既定値 |
| `Slider` | 範囲を持つ数値のスライダー表示 |
| `List` | 配列・コンテナの一覧表示 |
| `Curve` | Curve 型専用の編集UI |
| `Hidden` | YAML対象として登録するがInspectorには表示しない値 |

`Auto` では少なくとも `bool`、`int`、`float`、`std::string`、`Vector2`、`Vector3`、`Vector4`、`Color`、`Matrix3x3`、`Matrix4x4` を扱う。`enum class` は選択肢の名前を C++ から自動取得できないため、後に `EnumTraits<T>` で表示名を登録して List 表示する。

Button などの Action は Field ではなく保存対象にもならないため、初期設計には含めない。必要になった時点で、フィールド登録とは分離した Inspector 操作 API として追加する。

## Material Inspector

Renderer Component の Inspector は、保持している Material をスロットごとに表示する。Material は Shader Reflection が持つ Property 定義を全件 `FieldRegistry` 相当の共通描画処理へ渡す。

- float、int、bool、vector、matrix、color は型に応じて編集する。
- Texture2D は現在のプロジェクト相対パスを表示し、対応画像形式のファイル選択と解除を提供する。
- シェーダーコンパイル時に未使用として除去された定数 Property は、編集不可として表示する。
- SpriteRenderer など Renderer が毎フレーム値を供給する Property は、Inspector から変更しても次の供給時に上書きされる。この挙動は許容する。

Material Property の描画と型分岐は Material 側へ集約し、Renderer Component 側は保持 Material を選んで表示するだけにする。

RendererのMaterial参照は、シリアライズ用の`m_materialPath`／`m_materialPaths`を通常Fieldとして表示せず、`GetMaterialSlotCount()`が返す固定スロットとして表示する。利用者はスロットを追加できず、`Select Material`から`Assets/`配下の`.material`を割り当てる。

Material名は入力欄で編集し、Enterまたはフォーカス解除で確定する。未保存Materialには`Save Material As`、保存済みMaterialには`Save Material`を表示する。Material未設定または参照エラー時は、Shaderを選択して新しい未保存Materialを作成できる。

## 制約

- Componentフィールドは値の編集だけを担当し、ButtonなどのActionをFieldとして登録しない。
- GameObjectの生成・破棄と親変更はHierarchy、Component追加はInspectorの専用操作としてField編集から分離する。
- Component ごとの ImGui ID スコープは Inspector Window 側が作成し、同名フィールドを持つ複数 Component の衝突を防ぐ。
- `RegisterFields()` は Inspector・保存・読込のたびに再実行する。一時的な参照を永続保持しない。
