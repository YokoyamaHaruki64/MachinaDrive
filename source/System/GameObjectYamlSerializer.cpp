#include "pch.h"

#include "System/GameObjectYamlSerializer.h"

#include <unordered_map>
#include <unordered_set>

#include "Component/Transform.h"
#include "System/ComponentRegistry.h"
#include "System/GameObject.h"
#include "System/Logger.h"
#include "System/YAMLDocumentIO.h"
#include "System/YAMLValueConverter.h"

bool GameObjectYamlSerializer::ReadRequiredString(const YAMLValue& map, std::string_view key, std::string& output)
{
    const YAMLValue* value = map.Find(key);
    if (!value || value->type != YAMLValue::Type::Scalar)
    {
        Logger::Error("GameObject文書の必須文字列が不正です: " + std::string(key));
        return false;
    }

    output = value->scalar;
    return true;
}

template <typename T>
bool GameObjectYamlSerializer::ReadRequiredValue(const YAMLValue& map, std::string_view key, T& output)
{
    const YAMLValue* value = map.Find(key);
    if (!value || !YAMLValueConverter<T>::FromYAMLValue(*value, output, {}).success)
    {
        Logger::Error("GameObject文書の必須値が不正です: " + std::string(key));
        return false;
    }
    return true;
}

template <typename T>
YAMLValue GameObjectYamlSerializer::ToYAMLValue(const T& value)
{
    YAMLValue output;
    YAMLValueConverter<T>::ToYAMLValue(value, output, {});
    return output;
}

YAMLValue GameObjectYamlSerializer::ComponentToYAMLValue(
    Component& component,
    const YAMLValueConversionContext& context)
{
    // 実際のC++型ではなくRegistryの安定した型名をファイルへ保存する。
    YAMLValue output = YAMLValue::Map();
    const ComponentRegistration* registration = ComponentRegistry::Find(typeid(component));
    if (!registration)
    {
        Logger::Error("未登録Componentは保存できません: " + std::string(typeid(component).name()));
        return YAMLValue::Null();
    }

    output.Add("type", YAMLValue::Scalar(registration->typeName, true));

    // ComponentのフィールドをYAMLValueに変換する
    YAMLValue fields;
    if (!component.ToYAMLValue(fields, context))
    {
        // ToYAMLValueの失敗は、Componentの内部エラーであり、保存できないことを意味する
        // YAMLValueConverterの未実装型などが原因となるが、Reflect<T>::Members()をマクロにより定義することでYAML変換を実装できる。
        return YAMLValue::Null();
    }
    output.Add("fields", std::move(fields));
    return output;
}

bool GameObjectYamlSerializer::CreateComponentsFromYAMLValue(
    const YAMLValue& source,
    GameObject& gameObject,
    std::vector<PendingComponent>& pendingComponents)
{
    // sourceはGameObject単体のYAMLValueであるため、componentsキーの存在と型を検証する。
    const YAMLValue* components = source.Find("components");
    if (!components || components->type != YAMLValue::Type::Sequence)
    {
        Logger::Error("GameObjectのcomponentsが不正です: " + gameObject.GetName());
        return false;
    }

    // Component配列を復元
    for (const YAMLValue& componentValue : components->sequence)
    {
        if (componentValue.type != YAMLValue::Type::Map)
        {
            Logger::Error("ComponentエントリがMapではありません: " + gameObject.GetName());
            continue;
        }

        // Componentの型名を必須キーとして取得する。未登録型はスキップする。
        std::string typeName;
        if (!ReadRequiredString(componentValue, "type", typeName))
        {
            continue;
        }

        // Componentのフィールドを必須キーとして取得する。Mapでない場合はエラーとしてスキップする。
        const YAMLValue* fields = componentValue.Find("fields");
        if (!fields || fields->type != YAMLValue::Type::Map)
        {
            Logger::Error("Componentのfieldsが不正です: " + typeName);
            continue;
        }

        // Componentを生成する。
        Component* component{};
        const ComponentRegistration* registration = ComponentRegistry::Find(typeName);
        if (!registration)
        {
            Logger::Error("未登録Componentをスキップしました: " + typeName);
            continue;
        }

        // GameObject生成時に必ず存在するTransformは重複追加せず、既存のComponentを使う。
        if (registration->type == typeid(Transform))
        {
            component = gameObject.GetTransform();
        }
        else
        {
            component = ComponentRegistry::Add(*registration, gameObject);
        }

        if (!component)
        {
            Logger::Error("Componentを生成できません: " + typeName);
            continue;
        }
        // ComponentとfieldのYAMLValueをPendingComponentに登録する。Field復元はOnStart前にまとめて行う。
        pendingComponents.push_back({ component, fields });
    }
    return true;
}

GameObjectLoadResult::~GameObjectLoadResult()
{
    // Managerへ移管されなかった読込済みObjectにも、通常の破棄通知を保証する。
    for (const std::unique_ptr<GameObject>& gameObject : gameObjects)
    {
        if (gameObject)
        {
            gameObject->OnDestroy();
        }
    }
}

GameObjectLoadResult::GameObjectLoadResult(GameObjectLoadResult&&) noexcept = default;
GameObjectLoadResult& GameObjectLoadResult::operator=(GameObjectLoadResult&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    for (const std::unique_ptr<GameObject>& gameObject : gameObjects)
    {
        if (gameObject)
        {
            gameObject->OnDestroy();
        }
    }
    name = std::move(other.name);
    gameObjects = std::move(other.gameObjects);
    return *this;
}

bool GameObjectYamlSerializer::SaveToFile(
    const std::filesystem::path& path,
    std::string_view name,
    const std::vector<GameObject*>& gameObjects,
    PrefabReferenceSaveMode prefabReferenceMode)
{
    // 実行時ポインタを永続化しないため、保存範囲内だけで有効な連番IDを割り当てる。
    std::unordered_map<const GameObject*, ObjectId> objectIds;
    objectIds.reserve(gameObjects.size());
    for (size_t index = 0; index < gameObjects.size(); ++index)
    {
        objectIds.emplace(gameObjects[index], static_cast<ObjectId>(index + 1));
    }

    YAMLValueConversionContext context;
    context.getObjectId = [&objectIds](const GameObject* gameObject) -> std::optional<ObjectId>
    {
        const auto found = objectIds.find(gameObject);
        return found != objectIds.end() ? std::optional<ObjectId>(found->second) : std::nullopt;
    };

    // Scene/Prefab文書全体を組み立ててから一度だけ書き込む
    // このdocumentがファイルのルートにあたる
    YAMLValue document = YAMLValue::Map();
    document.Add("version", ToYAMLValue(FileVersion));
    document.Add("name", YAMLValue::Scalar(std::string(name), true));

    // GameObjectの配列を保存する
    YAMLValue objects = YAMLValue::Sequence();
    objects.sequence.reserve(gameObjects.size());
    for (GameObject* gameObject : gameObjects)
    {
        // GameObjectのID、名前、Prefab参照、親ID、Component配列を保存する
        YAMLValue object = YAMLValue::Map();
        object.Add("id", ToYAMLValue(objectIds.at(gameObject)));
        object.Add("name", YAMLValue::Scalar(gameObject->GetName(), true));

        if (prefabReferenceMode == PrefabReferenceSaveMode::Include && !gameObject->GetPrefabPath().empty())
        {
            object.Add("prefab", YAMLValue::Scalar(gameObject->GetPrefabPath(), true));
        }
        else
        {
            // Prefab参照を保存しない場合はNullを明示する
            object.Add("prefab", YAMLValue::Null());
        }

        // 親TransformのGameObjectを保存
        Transform* parent = gameObject->GetTransform()->GetParent();
        // 親のIDをGameObject*から検索、見つからなければRoot扱いとしてNullを保存する
        const auto parentId = parent ? objectIds.find(parent->GetOwner()) : objectIds.end();
        object.Add("parent", parentId != objectIds.end()
            ? ToYAMLValue(parentId->second)
            : YAMLValue::Null());

        // Component配列を保存
        YAMLValue components = YAMLValue::Sequence();
        for (Component* component : gameObject->GetComponents())
        {
            YAMLValue encoded = ComponentToYAMLValue(*component, context);
            if (encoded.type != YAMLValue::Type::Null)
            {
                components.sequence.emplace_back(std::move(encoded));
            }
        }
        object.Add("components", std::move(components));
        objects.sequence.emplace_back(std::move(object));
    }
    document.Add("gameObjects", std::move(objects));

    return YAMLDocumentIO::Save(path, document);
}

bool GameObjectYamlSerializer::LoadFromFile(
    const std::filesystem::path& path,
    GameObjectLoadResult& output,
    PrefabSourcePathMode sourcePathMode)
{
    // 解析失敗時に呼び出し側のSceneを変更しないよう、復元結果はローカルに保持する。
    YAMLValue document;
    if (!YAMLDocumentIO::Load(path, document) || document.type != YAMLValue::Type::Map)
    {
        return false;
    }

    // バージョンと名前を検証する。バージョンが異なる場合は互換性がないためロードを中止する。
    uint32_t version{};
    std::string name;
    if (!ReadRequiredValue(document, "version", version) ||
        version != FileVersion ||
        !ReadRequiredString(document, "name", name))
    {
        Logger::Error("未対応または不正なGameObject文書です: " + path.string());
        return false;
    }

    // gameObjectsキーを検証する
    const YAMLValue* sourceObjects = document.Find("gameObjects");
    if (!sourceObjects || sourceObjects->type != YAMLValue::Type::Sequence)
    {
        Logger::Error("gameObjectsがSequenceではありません: " + path.string());
        return false;
    }

    GameObjectLoadResult loaded;
    loaded.name = std::move(name);
    loaded.gameObjects.reserve(sourceObjects->sequence.size());

    // ID→GameObject*のマップを作成し、親子関係や参照解決に使う
    std::unordered_map<ObjectId, GameObject*> objectsById;
    // ComponentのField復元をOnStart前にまとめて行うため、PendingComponentに登録する
    std::vector<PendingComponent> pendingComponents;
    // 親子関係の復元をOnStart前にまとめて行うため、PendingParentに登録する
    std::vector<PendingParent> pendingParents;

    // 参照解決より先に、ファイル内の全GameObjectを生成する。
    for (const YAMLValue& sourceObject : sourceObjects->sequence)
    {
        if (sourceObject.type != YAMLValue::Type::Map)
        {
            Logger::Error("GameObjectエントリがMapではありません: " + path.string());
            return false;
        }

        // GameObjectのIDと名前を必須キーとして取得する。IDは0を許容せず、重複も許容しない。
        ObjectId id{};
        std::string objectName;
        if (!ReadRequiredValue(sourceObject, "id", id) ||
            !ReadRequiredString(sourceObject, "name", objectName) ||
            id == 0 || objectsById.contains(id))
        {
            Logger::Error("GameObject IDまたはnameが不正です: " + path.string());
            return false;
        }

        // GameObjectを生成
        std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>(std::move(objectName));
        GameObject* gameObjectPointer = gameObject.get();
        if (sourcePathMode == PrefabSourcePathMode::UseSourceFilePath)
        {// .prefabはファイル自身がPrefabのソースであるため、Prefab参照を保存する。
            gameObjectPointer->SetPrefabPath(path.generic_string());
        }
        else if (const YAMLValue* prefab = sourceObject.Find("prefab"); prefab && prefab->type == YAMLValue::Type::Scalar)
        {
            // Prefab参照が存在する場合は、ファイルの存在を確認して解決する。存在しない場合はエラーとしてマークする。
            if (std::filesystem::exists(prefab->scalar))
            {
                gameObjectPointer->SetPrefabPath(prefab->scalar);
            }
            else
            {
                gameObjectPointer->MarkPrefabResolutionError(prefab->scalar);
                Logger::Error("Prefab参照を解決できません: " + prefab->scalar);
            }
        }

        objectsById.emplace(id, gameObjectPointer);
        loaded.gameObjects.emplace_back(std::move(gameObject));

        // 親子関係の復元
        std::optional<ObjectId> parentId;
        // Type::NullはRoot扱いとして、親IDを持たないことを意味する。
        if (const YAMLValue* parent = sourceObject.Find("parent"); parent && parent->type != YAMLValue::Type::Null)
        {
            ObjectId decodedParent{};
            // 整数型ConverterでIDを復元する
            if (!YAMLValueConverter<ObjectId>::FromYAMLValue(*parent, decodedParent, {}).success)
            {
                Logger::Error("GameObjectのparentが不正です: " + gameObjectPointer->GetName());
                return false;
            }
            parentId = decodedParent;
        }
        pendingParents.push_back({ gameObjectPointer, parentId });
    }

    // Componentをすべて生成してからFieldを復元し、Component間依存をOnStart前に揃える。
    for (size_t index = 0; index < sourceObjects->sequence.size(); ++index)
    {
        if (!CreateComponentsFromYAMLValue(
            sourceObjects->sequence[index],
            *loaded.gameObjects[index],
            pendingComponents))
        {
            return false;
        }
    }

    // Field復元用のコンテキストを作成する
    YAMLValueConversionContext context;
    // GameObject参照のID→ポインタ解決を行う
    context.resolveObject = [&objectsById](ObjectId id)
    {
        const auto found = objectsById.find(id);
        return found != objectsById.end() ? found->second : nullptr;
    };
    // GameObject参照のポインタ→ID解決を行う。未登録ポインタはstd::nulloptを返す。
    context.resolvePrefab = [](std::string_view prefabPath)
    {
        return std::filesystem::exists(prefabPath);
    };

    // Field復元後に親子とGameObject参照を解決する。これにより順序依存を持ち込まない。
    for (const PendingComponent& pending : pendingComponents)
    {
        // ComponentのFieldを復元する
        if (!pending.component->FromYAMLValue(*pending.fields, context))
        {
            Logger::Error("Component Fieldの復元に失敗しました: " + pending.component->GetOwner()->GetName());
        }
    }

    // 親子関係を復元する。親IDが存在しない場合はRoot扱いとしてスキップする。
    for (const PendingParent& pending : pendingParents)
    {
        if (!pending.parentId)
        {
            continue;
        }

        const auto parent = objectsById.find(*pending.parentId);
        if (parent == objectsById.end() || !pending.child->SetParent(parent->second, ParentTransformMode::KeepLocal))
        {
            Logger::Error("GameObjectの親参照を解決できません: " + pending.child->GetName());
        }
    }

    output = std::move(loaded);
    return true;
}
