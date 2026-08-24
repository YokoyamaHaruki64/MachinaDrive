#include "pch.h"

#include "System/GameObjectArchive.h"

#include <unordered_map>
#include <unordered_set>

#include "Component/Transform.h"
#include "System/ComponentRegistry.h"
#include "System/GameObject.h"
#include "System/Logger.h"
#include "System/YAMLArchive.h"
#include "System/YAMLCodec.h"

class GameObjectArchive::Implementation final
{
public:
    static constexpr uint32_t Version = 1;
    using ObjectId = YAMLArchiveContext::ObjectId;

    struct PendingComponent
    {
        Component* component{};
        const YAMLValue* fields{};
    };

    struct PendingParent
    {
        GameObject* child{};
        std::optional<ObjectId> parentId;
    };

    static bool DecodeRequiredString(const YAMLValue& map, std::string_view key, std::string& output)
    {
        const YAMLValue* value = map.Find(key);
        if (!value || value->type != YAMLValue::Type::Scalar)
        {
            Logger::Error("GameObject Archiveの必須文字列が不正です: " + std::string(key));
            return false;
        }

        output = value->scalar;
        return true;
    }

    template <typename T>
    static bool DecodeRequired(const YAMLValue& map, std::string_view key, T& output)
    {
        const YAMLValue* value = map.Find(key);
        if (!value || !YAMLCodec<T>::Decode(*value, output, {}).success)
        {
            Logger::Error("GameObject Archiveの必須値が不正です: " + std::string(key));
            return false;
        }
        return true;
    }

    template <typename T>
    static YAMLValue Encode(const T& value)
    {
        YAMLValue output;
        YAMLCodec<T>::Encode(value, output, {});
        return output;
    }

    static YAMLValue EncodeComponent(
        Component& component,
        const YAMLArchiveContext& context)
    {
        YAMLValue output = YAMLValue::Map();
        const ComponentRegistration* registration = ComponentRegistry::Find(typeid(component));
        if (!registration)
        {
            Logger::Error("未登録Componentは保存できません: " + std::string(typeid(component).name()));
            return YAMLValue::Null();
        }

        output.Add("type", YAMLValue::Scalar(registration->typeName, true));

        YAMLValue fields;
        if (!component.EncodeFields(fields, context))
        {
            return YAMLValue::Null();
        }
        output.Add("fields", std::move(fields));
        return output;
    }

    static bool DecodeComponents(
        const YAMLValue& source,
        GameObject& gameObject,
        std::vector<PendingComponent>& pendingComponents)
    {
        const YAMLValue* components = source.Find("components");
        if (!components || components->type != YAMLValue::Type::Sequence)
        {
            Logger::Error("GameObjectのcomponentsが不正です: " + gameObject.GetName());
            return false;
        }

        for (const YAMLValue& componentValue : components->sequence)
        {
            if (componentValue.type != YAMLValue::Type::Map)
            {
                Logger::Error("ComponentエントリがMapではありません: " + gameObject.GetName());
                continue;
            }

            std::string typeName;
            if (!DecodeRequiredString(componentValue, "type", typeName))
            {
                continue;
            }

            const YAMLValue* fields = componentValue.Find("fields");
            if (!fields || fields->type != YAMLValue::Type::Map)
            {
                Logger::Error("Componentのfieldsが不正です: " + typeName);
                continue;
            }

            Component* component{};
            const ComponentRegistration* registration = ComponentRegistry::Find(typeName);
            if (!registration)
            {
                Logger::Error("未登録Componentをスキップしました: " + typeName);
                continue;
            }

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
            pendingComponents.push_back({ component, fields });
        }
        return true;
    }
};

LoadedGameObjectArchive::~LoadedGameObjectArchive()
{
    for (const std::unique_ptr<GameObject>& gameObject : gameObjects)
    {
        if (gameObject)
        {
            gameObject->OnDestroy();
        }
    }
}

LoadedGameObjectArchive::LoadedGameObjectArchive(LoadedGameObjectArchive&&) noexcept = default;
LoadedGameObjectArchive& LoadedGameObjectArchive::operator=(LoadedGameObjectArchive&& other) noexcept
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

bool GameObjectArchive::Save(
    const std::filesystem::path& path,
    std::string_view name,
    const std::vector<GameObject*>& gameObjects,
    bool includePrefabReferences)
{
    std::unordered_map<const GameObject*, Implementation::ObjectId> objectIds;
    objectIds.reserve(gameObjects.size());
    for (size_t index = 0; index < gameObjects.size(); ++index)
    {
        objectIds.emplace(gameObjects[index], static_cast<Implementation::ObjectId>(index + 1));
    }

    YAMLArchiveContext context;
    context.getObjectId = [&objectIds](const GameObject* gameObject) -> std::optional<Implementation::ObjectId>
    {
        const auto found = objectIds.find(gameObject);
        return found != objectIds.end() ? std::optional<Implementation::ObjectId>(found->second) : std::nullopt;
    };

    YAMLValue document = YAMLValue::Map();
    document.Add("version", Implementation::Encode(Implementation::Version));
    document.Add("name", YAMLValue::Scalar(std::string(name), true));

    YAMLValue objects = YAMLValue::Sequence();
    objects.sequence.reserve(gameObjects.size());
    for (GameObject* gameObject : gameObjects)
    {
        YAMLValue object = YAMLValue::Map();
        object.Add("id", Implementation::Encode(objectIds.at(gameObject)));
        object.Add("name", YAMLValue::Scalar(gameObject->GetName(), true));

        if (includePrefabReferences && !gameObject->GetPrefabPath().empty())
        {
            object.Add("prefab", YAMLValue::Scalar(gameObject->GetPrefabPath(), true));
        }
        else
        {
            object.Add("prefab", YAMLValue::Null());
        }

        Transform* parent = gameObject->GetTransform()->GetParent();
        const auto parentId = parent ? objectIds.find(parent->GetOwner()) : objectIds.end();
        object.Add("parent", parentId != objectIds.end()
            ? Implementation::Encode(parentId->second)
            : YAMLValue::Null());

        YAMLValue components = YAMLValue::Sequence();
        for (Component* component : gameObject->GetComponents())
        {
            YAMLValue encoded = Implementation::EncodeComponent(*component, context);
            if (encoded.type != YAMLValue::Type::Null)
            {
                components.sequence.emplace_back(std::move(encoded));
            }
        }
        object.Add("components", std::move(components));
        objects.sequence.emplace_back(std::move(object));
    }
    document.Add("gameObjects", std::move(objects));

    return YAMLArchive::SaveDocument(path, document);
}

bool GameObjectArchive::Load(
    const std::filesystem::path& path,
    LoadedGameObjectArchive& output,
    bool applySourcePrefabPath)
{
    YAMLValue document;
    if (!YAMLArchive::LoadDocument(path, document) || document.type != YAMLValue::Type::Map)
    {
        return false;
    }

    uint32_t version{};
    std::string name;
    if (!Implementation::DecodeRequired(document, "version", version) ||
        version != Implementation::Version ||
        !Implementation::DecodeRequiredString(document, "name", name))
    {
        Logger::Error("未対応または不正なGameObject Archiveです: " + path.string());
        return false;
    }

    const YAMLValue* sourceObjects = document.Find("gameObjects");
    if (!sourceObjects || sourceObjects->type != YAMLValue::Type::Sequence)
    {
        Logger::Error("gameObjectsがSequenceではありません: " + path.string());
        return false;
    }

    LoadedGameObjectArchive loaded;
    loaded.name = std::move(name);
    loaded.gameObjects.reserve(sourceObjects->sequence.size());

    std::unordered_map<Implementation::ObjectId, GameObject*> objectsById;
    std::vector<Implementation::PendingComponent> pendingComponents;
    std::vector<Implementation::PendingParent> pendingParents;

    // 参照解決より先に、ファイル内の全GameObjectを生成する。
    for (const YAMLValue& sourceObject : sourceObjects->sequence)
    {
        if (sourceObject.type != YAMLValue::Type::Map)
        {
            Logger::Error("GameObjectエントリがMapではありません: " + path.string());
            return false;
        }

        Implementation::ObjectId id{};
        std::string objectName;
        if (!Implementation::DecodeRequired(sourceObject, "id", id) ||
            !Implementation::DecodeRequiredString(sourceObject, "name", objectName) ||
            id == 0 || objectsById.contains(id))
        {
            Logger::Error("GameObject IDまたはnameが不正です: " + path.string());
            return false;
        }

        std::unique_ptr<GameObject> gameObject = std::make_unique<GameObject>(std::move(objectName));
        GameObject* gameObjectPointer = gameObject.get();
        if (applySourcePrefabPath)
        {
            gameObjectPointer->SetPrefabPath(path.generic_string());
        }
        else if (const YAMLValue* prefab = sourceObject.Find("prefab"); prefab && prefab->type == YAMLValue::Type::Scalar)
        {
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

        std::optional<Implementation::ObjectId> parentId;
        if (const YAMLValue* parent = sourceObject.Find("parent"); parent && parent->type != YAMLValue::Type::Null)
        {
            Implementation::ObjectId decodedParent{};
            if (!YAMLCodec<Implementation::ObjectId>::Decode(*parent, decodedParent, {}).success)
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
        if (!Implementation::DecodeComponents(
            sourceObjects->sequence[index],
            *loaded.gameObjects[index],
            pendingComponents))
        {
            return false;
        }
    }

    YAMLArchiveContext context;
    context.resolveObject = [&objectsById](Implementation::ObjectId id)
    {
        const auto found = objectsById.find(id);
        return found != objectsById.end() ? found->second : nullptr;
    };
    context.resolvePrefab = [](std::string_view prefabPath)
    {
        return std::filesystem::exists(prefabPath);
    };
    for (const Implementation::PendingComponent& pending : pendingComponents)
    {
        if (!pending.component->DecodeFields(*pending.fields, context))
        {
            Logger::Error("Component Fieldの復元に失敗しました: " + pending.component->GetOwner()->GetName());
        }
    }

    for (const Implementation::PendingParent& pending : pendingParents)
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
