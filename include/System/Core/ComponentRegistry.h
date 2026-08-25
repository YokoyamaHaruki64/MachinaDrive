#pragma once

#include <functional>
#include <string>
#include <typeindex>
#include <vector>

#include "System/Core/component.h"
#include "System/Core/Script.h"

class GameObject;

enum class ComponentCategory
{
    Component,
    Script,
};

struct ComponentRegistration
{
    std::string typeName;
    std::string displayName;
    std::type_index type{ typeid(void) };
    ComponentCategory category{ ComponentCategory::Component };
    bool allowMultiple = true;
    bool showInAddComponent = true;
    std::function<Component*(GameObject&)> factory;
};

class ComponentRegistry final
{
public:
    template <typename T>
    static void Register(
        std::string typeName,
        std::string displayName = {},
        bool allowMultiple = false,
        bool showInAddComponent = true)
    {
        static_assert(std::is_base_of_v<Component, T>, "Component派生型だけを登録できます。");
        static_assert(std::is_constructible_v<T, GameObject*>, "GameObject*で生成できるComponentだけを登録できます。");

        ComponentRegistration registration{
            std::move(typeName),
            displayName.empty() ? typeid(T).name() : std::move(displayName),
            typeid(T),
            std::is_base_of_v<Script, T> ? ComponentCategory::Script : ComponentCategory::Component,
            allowMultiple,
            showInAddComponent,
            [](GameObject& gameObject) -> Component*
            {
                return gameObject.AddComponent<T>();
            },
        };

        RegisterInternal(std::move(registration));
    }

    static const std::vector<ComponentRegistration>& GetRegistrations();
    static const ComponentRegistration* Find(std::type_index type);
    static const ComponentRegistration* Find(std::string_view typeName);
    static bool CanAdd(const ComponentRegistration& registration, const GameObject& gameObject);
    static Component* Add(const ComponentRegistration& registration, GameObject& gameObject);

private:
    static std::vector<ComponentRegistration>& GetMutableRegistrations();
    static void RegisterInternal(ComponentRegistration registration);
};

template <typename T>
class ComponentAutoRegistrar final
{
public:
    ComponentAutoRegistrar(
        std::string typeName,
        std::string displayName,
        bool allowMultiple = false,
        bool showInAddComponent = true)
    {
        ComponentRegistry::Register<T>(
            std::move(typeName),
            std::move(displayName),
            allowMultiple,
            showInAddComponent);
    }
};

// 通常は重複追加を許可しない。クラス定義の先頭に配置して自己登録する。
#define REGISTER_COMPONENT(type, displayName) \
    inline static const ComponentAutoRegistrar<type> s_componentRegistration{ #type, displayName }

#define REGISTER_COMPONENT_OPTIONS(type, displayName, allowMultiple, showInAddComponent) \
    inline static const ComponentAutoRegistrar<type> s_componentRegistration{ #type, displayName, allowMultiple, showInAddComponent }
