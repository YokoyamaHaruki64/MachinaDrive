#pragma once

#include "math/Vector3.h"
#include "System/Editor/FieldRegistry.h"

class Transform;
class GameObject;

class Component
{

protected:
	GameObject* m_owner = nullptr;
    bool m_isEnabled = true;

public:
	Component() = delete;
    explicit Component(GameObject* owner)
        : m_owner(owner)
    {
    }
    virtual ~Component() {}

    virtual void OnLoaded() {};
    virtual void OnEditorUpdate() {};
    virtual void OnStart() {};
    // GameObjectが有効になった遷移で呼び出される。
    virtual void OnActive() {};
    // Componentが有効になった遷移で呼び出される。
    virtual void OnEnable() {};
    virtual void OnDestroy() {};

	virtual void Update() {};
    virtual void LateUpdate() {};
	virtual void Draw() {};

    bool DrawInspectorImGui(InspectorFieldSpace inspectorSpace = InspectorFieldSpace::All);
    bool ToYAMLValue(YAMLValue& output, const YAMLValueConversionContext& context = {});
    bool FromYAMLValue(const YAMLValue& input, const YAMLValueConversionContext& context = {});

    Transform* transform() const;
    GameObject* GetOwner() const { return m_owner; }

    bool IsEnabled() const { return m_isEnabled; }
    void SetEnabled(bool enabled);

    template <typename T>
    T* GetComponent() const;

protected:
    virtual void RegisterFields(FieldRegistry& fields) {}
};
