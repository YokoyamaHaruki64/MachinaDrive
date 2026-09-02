# このフレームワークでのゲーム開発

## 目的

この資料は、ゲーム固有の機能を追加するときの標準的な流れをまとめる。フレームワーク側の再利用可能な機能は`Component`、ゲームごとに変わる振る舞いは`Script`として実装する。実行中の調整はInspector、保存はScene／Prefab／MaterialのYAMLアセットを使う。

全体の実行順、所有関係、Transformの規約は[ARCHITECTURE.md](ARCHITECTURE.md)、Editor／Inspectorとフィールド登録の詳細は[EDITOR.md](EDITOR.md)、保存形式は[SERIALIZATION.md](SERIALIZATION.md)を参照する。

## Editor と Runtime

Editorでの調整は`GM_ENABLE_EDITOR=1`のDebug構成で行う。Game画面はGame RenderTextureを`GameWindow`へ表示し、Edit中はFree、Play中は16:9の表示比率で表示する。SceneやPrefabは通常どおり共通のYAMLアセットとして保存する。

出荷時は`GM_ENABLE_EDITOR=0`のRelease構成を使用する。この構成ではImGui、Docking、Inspector、GameWindowを初期化せず、ImGui本体とbackendもコンパイル対象から除外する。ゲーム描画はGame RenderTextureからBackBufferへ転送されるため、PrefabやSceneをRuntime用に変換する作業は不要である。

## 開発の基本単位

| 作りたいもの | 置き場所 | 基底型 | 例 |
| --- | --- | --- | --- |
| 複数ゲームで再利用する機能 | `include/Component/`、`source/Component/` | `Component` または `RendererBase` | Camera、SpriteRenderer、Audio |
| 今回のゲーム固有の振る舞い | `include/Script/`、`source/Script/` | `Script` | Player、Enemy、UI制御 |
| シーンの初期構築・進行の入口 | `include/Scene/`、`source/Scene/` | `Scene` | GameScene、TitleScene |
| 配置・再利用するデータ | `Assets/` | YAMLアセット | `.scene`、`.prefab`、`.material` |

`Script`は`Component`を継承する。GameObjectはTransformを必ず1つ持ち、追加したComponent／Scriptを所有する。Scriptからは`transform()`で自身のTransform、`GetComponent<T>()`で同一GameObjectのComponentへアクセスする。

## 標準的な作業フロー

1. 機能の責務を決める。汎用機能ならComponent、ゲームルールや演出ならScriptにする。
2. `include/Script/`と`source/Script/`に同名の`.h`／`.cpp`を作り、Visual StudioプロジェクトとFilterの`Script`へ追加する。
3. Scriptを`Script`から継承し、`System/GameplayMacros.h`が提供する`GM_SCRIPT(Type, "表示名")`をクラス定義の先頭で宣言する。これによりInspectorの`Add Component`とYAML復元から型を利用できる。重複追加や一覧表示を変えたい場合は`GM_SCRIPT_OPTION(Type, "表示名", allowMultiple, showInAddComponent)`を使う。
4. Inspectorで調整・保存したいメンバーを`USE_FIELDS`と`FIELDS_BEGIN`／`FIELDS_END`内の`GM_FIELD`で登録する。
5. `OnStart()`で同一GameObjectの必要Component取得や、シーン内参照の初期解決を行う。全Prefabの復元とフィールド適用は`OnStart()`より先に完了する。開始時に有効なObject／Componentでは、その後に`OnActive()`／`OnEnable()`が呼び出される。
6. `Update()`には毎フレームのゲームロジック、`LateUpdate()`には他のUpdate結果を利用する後段処理を書く。時間に依存する量は`Time::GetDeltaTime()`を掛ける。
7. Hierarchy／InspectorでGameObjectへ追加して値を調整する。配置を残すならScene保存、再利用したい階層ならPrefab保存を使う。

## 手本: SampleMoveScript

[SampleMoveScript.h](../include/Script/SampleMoveScript.h) と [SampleMoveScript.cpp](../source/Script/SampleMoveScript.cpp) は、そのままビルド対象になる実践的なScript例である。GameObjectへ追加すると、WASDで移動し、Shiftでダッシュ、Fで設定済みPrefabを生成する。Inspectorから設定、参照、Prefabパスを調整でき、すべてPrefab／Scene保存時にYAMLへ書き出される。

ヘッダーでは、登録マクロ、所有者を受け取るコンストラクタ、使用するライフサイクルフックを宣言する。

```cpp
class SampleMoveScript final : public Script
{
    GM_SCRIPT(SampleMoveScript, "Sample Move Script")

    void OnStart() override;
    void Update() override;
    void LateUpdate() override;

    USE_FIELDS

    SampleMoveSettings m_settings;
    GameObjectReference m_lookTarget;
    PrefabReference m_spawnPrefab;
    Audio* m_audio = nullptr;
};
```

`SampleMoveSettings`は`REFLECT_BEGIN`／`REFLECT_FIELD`／`REFLECT_END`で登録した独自型である。Script本体は`GM_FIELD(m_settings)`の1行だけで、ネストした各設定値をInspectorとYAMLへ再帰的に接続できる。privateメンバーを登録するため、設定型には`REFLECT_FRIEND(SampleMoveSettings)`を置く。

cppでは、`ComponentInc.h`を実装側の共通入口として使う。`OnStart()`で任意のAudioを取得し、`Update()`ではInput・時間・Prefab生成、`LateUpdate()`では参照先を使う後段の向き制御を行う。

```cpp
FIELDS_BEGIN(SampleMoveScript)
    GM_FIELD(m_settings);
    GM_FIELD(m_lookTarget);
    GM_FIELD(m_spawnPrefab);
FIELDS_END()

void SampleMoveScript::Update()
{
    const Vector3 direction = BuildMoveDirection(); // WASD入力
    float moveSpeed = m_settings.GetMoveSpeed();
    if (m_settings.IsDashAllowed() && Input::GetKeyPress(Input::KK_SHIFT))
    {
        moveSpeed *= m_settings.GetDashMultiplier();
    }

    transform()->Translate(direction * (moveSpeed * Time::GetDeltaTime()));

    if (Input::GetKeyTrigger(Input::KK_F) && m_spawnPrefab)
    {
        const Vector3 spawnPosition = transform()->GetPosition() + m_settings.GetSpawnOffset();
        Manager::Instantiate(m_spawnPrefab.GetPath(), spawnPosition, Vector3::Zero(), Vector3::One());
    }
}
```

`GM_FIELD`に登録しない値はInspectorにもYAMLにも含まれない。実行時キャッシュ、取得済みComponentポインタ、一時タイマーなどは通常登録しない。

ファイルパスを設定値として持つ場合は、通常の`GM_FIELD`ではなく`GM_FIELD_PATH`を使う。Inspectorから直接文字列を編集せず、初期フォルダと許可拡張子を指定したファイルダイアログで`Assets/`配下のファイルを選択する。`PrefabReference`はPrefab専用の同じ選択規則を自動で使用する。

## 有効状態とライフサイクル

GameObjectとComponentは、それぞれ実行時の有効状態を持つ。Inspectorの`Active`と`Enabled`、または次のアクセサから切り替える。

```cpp
gameObject->SetActive(false);
component->SetEnabled(false);

if (gameObject->IsActive() && component->IsEnabled())
{
    // 有効なComponentのUpdate／LateUpdateが実行される
}
```

GameObjectが無効な間は、そのGameObjectに属するComponentの`Update()`、`LateUpdate()`、Renderer描画を実行しない。Componentが無効な場合も、そのComponentの更新とRenderer描画を実行しない。独自の有効化時処理が必要なComponentは`OnActive()`、Component自身の有効化時処理が必要な場合は`OnEnable()`をオーバーライドする。`OnEnable()`はComponentと所有GameObjectの両方が有効になった時だけ呼び出される。

## 参照と生成

別GameObjectへの保持参照には生ポインタをメンバーとして残さず、`GameObjectReference`を使う。対象が`Destroy()`された場合に自動でnullになるため、毎フレーム利用する時も安全に判定できる。

```cpp
GameObjectReference m_target;

if (m_target)
{
    const Vector3 targetPosition = m_target->GetTransform()->GetPosition();
}
```

`GameObjectReference`も`GM_FIELD(m_target)`で登録でき、同じScene／Prefab内の参照として復元される。未設定は`None`、保存データに指定された参照先を解決できない場合はErrorLogとInspectorの`Error`で確認する。

新しいObjectは`Manager::CreateEmpty()`または`Manager::Instantiate()`で生成する。

```cpp
GameObject* effect = Manager::CreateEmpty("Hit Effect", position);
effect->AddComponent<SpriteRenderer>();

GameObject* enemy = Manager::Instantiate(
    "Assets/Prefabs/Enemy.prefab",
    position,
    Vector3::Zero(),
    Vector3::One());
```

通常フレーム中の`CreateEmpty()`はただちに開始される。Scene初期化中とPrefabロード中に生成されたObjectは、同じ生成バッチのObject群とComponent群が揃った後に`OnStart()`される。そのため、`OnStart()`で依存先を検索できる。

## Inspectorと保存

Inspectorの`Add Component`には`REGISTER_COMPONENT`済みのComponentとScriptが表示される。`GM_FIELD`の基本形は一変数につき一行である。

```cpp
void Enemy::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_hitPoint);
    GM_FIELD(m_moveSpeed);
    GM_FIELD(m_target);
}
```

独自の設定型も、`REFLECT_BEGIN`／`REFLECT_FIELD`／`REFLECT_END`でReflect登録すれば、`GM_FIELD(m_settings)`の1行でInspector表示とYAML保存に再帰利用できる。privateメンバーをReflectする型には`REFLECT_FRIEND(Type)`を置く。

TransformはWorld値を標準APIで扱う。`GetPosition()`／`SetPosition()`、`GetRotation()`／`SetRotation()`、`GetScale()`／`SetScale()`はWorld値、Local値が必要な場合だけ`GetLocal...()`／`SetLocal...()`を使う。回転の内部表現はQuaternionだが、Euler角を使う既存の`SetRotation(Vector3)`はラジアン入力で利用できる。

Sceneは各`Scene::Init()`で対応する`.scene`をロードし、`Manager::SaveCurrentScene()`で保存する。PrefabはHierarchyから作成し、生成済みPrefabインスタンスはInspectorの`Apply to Prefab`で更新する。MaterialはMaterial Inspectorから名前・Shader・Propertyを設定して`.material`として保存する。

## 実装時の注意

- `REGISTER_COMPONENT`はクラス定義の先頭に置く。コンストラクタは`GameObject*`を受け取れる形にする。
- `OnStart()`で取得した他Componentの生ポインタは、同一GameObjectの構成が不変である前提のキャッシュに限る。別GameObjectは`GameObjectReference`で保持する。
- Object生成直後に必要なComponentをすべて追加してから利用する。開始済みObjectへの`AddComponent()`は、そのComponentの`OnStart()`を即時実行する。
- `Destroy()`は破棄予約であり、同フレームの破棄フェーズまで実体は残る。親を破棄すると子孫にも予約が伝播する。
- 毎フレームのログは`Logger::Log`／`Warning`／`Error`を使う。停止が必要な回復不能な状態だけ`Logger::Fatal`を使う。
- アセットへの参照は`Assets/`からの相対パスで保持する。ローカル専用モデル・Texture・Soundは`Assets/Local/`に置き、Git管理しない。

## 実装後の確認

- 新規`.h`／`.cpp`を`.vcxproj`と`.vcxproj.filters`へ登録したか。
- Inspectorの`Add Component`でScriptを追加でき、フィールドを編集できるか。
- PrefabまたはSceneを保存・再読み込みして、登録フィールドと参照が復元されるか。
- 対象シーンを起動し、ログ・Performance・Hierarchyを確認したか。
- `Debug|x64`をビルドし、文字コードはUTF-8 BOM、改行はCRLFになっているか。
