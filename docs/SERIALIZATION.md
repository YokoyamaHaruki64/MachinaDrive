# シリアライズ設計

## 目的と範囲

実行時に構築したScene、Prefab、MaterialをYAML形式で保存・復元できるようにする。保存対象はInspectorのフィールド登録を正とし、同じ登録情報を編集UIとシリアライズで共有する。

初期実装では、Scene固有の初期化・更新処理を持つ既存の`Scene`クラスは残す。`.scene`はGameObject群の配置・構成を保持するデータファイルとして扱い、`Scene`クラスがロードとゲーム進行を担当する。PostEffectの保存は将来対応とする。

## 基本方針

- YAMLライブラリにはrapidyamlを使用する。ただし、ゲーム側のコードはrapidyamlのTreeを直接操作せず、`YAMLArchive`を介して読み書きする。
- 保存形式には`version`を持たせ、互換性が必要になった時はバージョンごとに読み込み処理を分岐できるようにする。
- 値の型はフィールド登録情報から決定する。YAMLへ型名は保存しない。
- 実行時のGPUハンドルやTextureHandleなどの一時IDは保存しない。再生成可能なプロジェクト相対パスを保存する。
- GameObject参照と親子関係は、ファイル内で一意な`id`で表す。`id`は実行時IDではなく、ファイルのロード時に解決するための値である。
- 参照値が未設定または`null`の場合は`None`として扱う。ファイルに参照先が明記されているにもかかわらず解決できない場合はErrorLogを記録し、Inspectorでは`Error`として表示する。

## 保存先

保存するアセットはGit管理される`Assets/`配下に置く。実行時に生成したPrefabも同じ場所へ保存できる。ローカル環境ごとに用意する大容量リソースは`Assets/Local/`配下に置き、Git管理しない。

| 種類 | 拡張子 | 基本配置 | Git管理 |
| --- | --- | --- | --- |
| Scene | `.scene` | `Assets/Scenes/` | 管理する |
| Prefab | `.prefab` | `Assets/Prefabs/` | 管理する |
| Material | `.material` | `Assets/Materials/` | 管理する |
| Shaderアセット・ソース | `.shader`、`.hlsl`、`.hlsli` | `Assets/Shaders/` | 管理する |
| ローカルリソース | モデル、Texture、Soundなど | `Assets/Local/` | 管理しない |

Shaderのコンパイル済み`.cso`は、従来どおり`BuildedShader/`へ出力する生成物とし、Git管理しない。

## Scene

SceneはGameObject群の状態を保存する。Prefabから生成したGameObjectも、初期実装ではコンポーネントとフィールドをすべてSceneへ保存する。これにより、Prefabが見つからない場合でもScene単体で復元できる。

`prefab`は生成元を示す任意の参照として保持する。Inspectorの`Apply to Prefab`は、この参照先のPrefabを更新する操作とする。Prefab変更を既存インスタンスへ自動反映する機能と、`Revert Prefab Overrides`は後続の対応とする。

```yaml
version: 1
name: Game
gameObjects:
  - id: 1
    name: Player
    prefab: Prefabs/Player.prefab
    parent: null
    components:
      - type: Transform
        fields:
          m_localPosition: [0.0, 0.0, 5.0]
          m_localRotation: [0.0, 0.0, 0.0, 1.0]
          m_localScale: [1.0, 1.0, 1.0]
```

`gameObjects`は親子の並び順に依存しない。ロードでは、全GameObjectとコンポーネントを生成してフィールドを適用してから、`parent`のIDを解決して親子関係を復元する。すべての復元が完了した後に`OnStart()`を呼ぶ。

### Sceneファイルの入出力

Scene基底クラスは、各Sceneに対応する一意の保存先を返す`GetSceneFilePath()`を持つ。各Sceneは`Assets/Scenes/`配下の`.scene`パスを返す。

```cpp
virtual const std::filesystem::path& GetSceneFilePath() const = 0;
```

GameObjectを所有するManagerが、実際の読み書きを担当する。`SaveCurrentScene()`は現在のSceneから`GetSceneFilePath()`を取得して保存し、呼び出し側がファイルパスを指定する必要はない。Sceneの`Init()`は`GetSceneFilePath()`を指定して`LoadSceneObjects()`を呼ぶ。

```cpp
bool Manager::SaveCurrentScene() const;
bool Manager::SaveSceneObjects(const std::filesystem::path& path) const;
bool Manager::LoadSceneObjects(const std::filesystem::path& path);
```

`LoadSceneObjects()`は、YAMLの解析と参照解決を完了してから既存のObject群を置き換える。解析・復元・参照解決のいずれかに失敗した場合はErrorLogを記録して`false`を返し、ロード前のObject群を維持する。Scene基底クラスに保存・読込用のvirtual関数は追加しない。

## Prefab

Prefabは再利用するGameObject階層のテンプレートである。単一GameObjectに限定せず、最初から`gameObjects`配列で保存する。これにより、子GameObjectを含むPrefabを同じ形式で扱える。

```yaml
version: 1
name: Player
gameObjects:
  - id: 1
    name: PlayerRoot
    parent: null
    components: []
  - id: 2
    name: Weapon
    parent: 1
    components: []
```

PrefabをInstantiateする時は、ファイル内の`id`を新しいGameObject群に対応付けて参照を解決する。ファイルの`id`を実行中のGameObjectへそのまま持ち込まない。

## Transformの親子関係

親子関係の正は`Transform`である。`Manager`は親子に関係なく全GameObjectを一元所有し、Transformは親と子への非所有参照だけを保持する。

TransformはLocal Position、Local Quaternion Rotation、Local Scaleを保存する。World行列は保存せず、ロード後にLocal変換と親のWorld行列から算出する。既存のEuler角APIは互換性のためLocal Euler角として扱うが、保存する回転値はQuaternionとする。

親変更時の`KeepWorld`と`KeepLocal`は実行時の操作規則であり、ファイルの復元時は保存済みのLocal変換を維持する`KeepLocal`で親を設定する。循環する`parent`参照や存在しないIDは読み込みエラーとして扱い、そのGameObjectを親なしとして復元する。

親に非一様Scale、子に回転がある組み合わせで生じるshearは、初期実装では完全には保存・復元しない。TRSへ近似分解する制約を維持する。

## Componentとフィールド

Componentは原則として`GameObject*`だけを受け取って生成する。モデルパス、Material、移動速度などの設定値をコンストラクタ引数として渡す生成方法は使用しない。ComponentFactoryは型名と生成関数を対応付け、各Componentの実装ファイルで`REGISTER_COMPONENT(Type)`を1行記述して登録する。

ゲーム固有の振る舞いは`Script`基底クラスを経由して`Component`を継承する。Scriptは別の生成・保存機構を持たず、通常のComponentと同じRegistryで復元する。InspectorのAdd Component一覧では、`Script`派生かどうかを基に`Components`と`Scripts`へ自動分類して表示する。

設定値は必ずInspectorフィールドとして登録し、通常生成、Prefab復元、Scene復元のいずれでも生成後に同じ経路で適用する。たとえばModelRendererはモデルのパスなどをフィールドとして保存し、フィールド適用後の`OnStart()`でリソースを解決する。コンストラクタ引数は、所有GameObjectや実行基盤などFieldとして保存できない不変の依存だけに限定する。

登録されていないComponent型、存在しないフィールド、期待する型と異なる値はエラーログを残して読み込みを継続する。必須のComponentが復元できない場合の扱いは、Componentごとに後続で定義する。

### 配列フィールド

`std::vector<T>`は、通常のフィールドと同じく`GM_FIELD(m_values)`の一行で登録する。FieldRegistryは配列と要素型を自動判定し、YAMLにはsequenceとして保存する。

```yaml
m_waypoints:
  - [0.0, 0.0, 0.0]
  - [5.0, 0.0, 0.0]
```

初期対応する要素型は`bool`、整数、`float`、`string`、Vector2、Vector3、Vector4、Color、Quaternionとする。Inspectorは折り畳み、要素編集、末尾追加、個別削除を提供する。要素の並び替えは後続対応とする。

`std::vector<GameObject*>`、`std::vector<PrefabReference>`、ネストした配列、`std::vector<bool>`は初期対応の対象外とする。`std::vector<bool>`は要素への通常の参照を持たない特殊なコンテナであり、FieldRegistryの共通処理に適さない。

### GameObjectとPrefabの参照

GameObject実体への参照とPrefabアセットへの参照は、別のField型として扱う。

| Field型 | 参照先 | YAMLに保存する値 | 用途 |
| --- | --- | --- | --- |
| `GameObjectReference` | Scene上のGameObject実体 | SceneまたはPrefab内の`id` | 追従対象、攻撃対象などの実体参照 |
| `PrefabReference` | Prefabアセット | `.prefab`のプロジェクト相対パス | 実行時にInstantiateするPrefabの指定 |

`GameObjectReference`は、対象がPrefabから生成されたObjectであっても、保存時にはPrefabパスではなくファイル内の`id`を参照する。ロードでは全GameObjectを生成した後の参照解決フェーズで`id`をポインタへ変換する。

Prefabファイルでは、同じPrefabに含まれるGameObjectだけを`id`で参照できる。Prefab外のScene上GameObjectを参照する値は保存できないため、保存時に`None`として扱い警告ログを記録する。ファイルに`id`が明記されているにもかかわらず対象が存在しない場合は、読み込み時にErrorLogを記録し、Inspectorでは`Error`として表示する。

フレームをまたいでGameObjectを保持するフィールドには、`GameObjectReference`を用いる。参照先GameObjectは自分を参照する`GameObjectReference`の非所有リストを持ち、破棄時にその全参照を自動解除する。`Instantiate()`や検索APIの返り値は、即時利用用の非所有`GameObject*`を維持する。

```cpp
class GameObjectReference
{
public:
    GameObjectReference& operator=(GameObject* target);
    GameObject* operator->() const;
    GameObject& operator*() const;
    explicit operator bool() const;

    GameObject* Get() const;
    void Set(GameObject* target);
    void Clear();
};
```

これにより、長期参照でも`m_target = object;`、`m_target->GetTransform()`、`if (m_target)`と書ける。ドット演算子はC++で転送できないため、GameObjectのメンバーへアクセスする時は`->`を用いる。暗黙の`GameObject*`変換は提供せず、関数へ渡す場合は`Get()`で明示する。

## Material

Materialは外部`.material`ファイルとして保存し、RendererはMaterialの全プロパティを埋め込まず、Materialファイルへの参照を保持する。

```yaml
version: 1
name: PlayerBody
shader: shader/DefaultLit.shader
properties:
  _BaseColor: [1.0, 1.0, 1.0, 1.0]
  _MainTex: Assets/Textures/player.png
```

`shader`はプロジェクト相対のシェーダーファイルパスを正とする。TextureプロパティもTextureHandleではなくソースファイルへの相対パスを保存する。RenderTextureなどのメモリ上だけに存在するTextureは初期実装ではシリアライズ対象外とし、警告ログを残して未設定として復元する。

`.material`自体または指定Shaderを解決できない場合は、RendererがMagentaFallbackMaterialを使用する。一方、Textureプロパティのパスだけを解決できない場合はMaterialをエラー扱いにせず、そのTextureだけをFallbackTextureへ置き換えて描画を継続する。この場合はErrorLogを記録し、Material Inspectorの該当Texture欄を`Error`として表示する。

## Inspectorとの関係

InspectorはGameObject名、登録済みComponentフィールド、Materialプロパティを編集する。Scene保存はScene全体、`Apply to Prefab`は選択GameObjectを含むPrefabを対象とする。保存UIと実際の書き込み処理は、YAMLArchiveおよび各アセットの保存サービスへ委譲する。

Button、Curveなど基本値以外のInspectorフィールド種別は、必要になった時点でシリアライズ形式と合わせて追加する。

