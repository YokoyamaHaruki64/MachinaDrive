#pragma once

#include "math/Vector3.h"
#include "System/Editor/FieldRegistry.h"

class Transform;
class GameObject;

class Component
{

protected:
	GameObject* m_owner = nullptr;

public:
	Component() = delete;
    explicit Component(GameObject* owner)
        : m_owner(owner)
    {
    }
    virtual ~Component() {}

    virtual void OnStart() {};
    virtual void OnDestroy() {};

	virtual void Update() {};
    virtual void LateUpdate() {};
	virtual void Draw() {};

    void DrawInspectorImGui(InspectorFieldSpace inspectorSpace = InspectorFieldSpace::All);
    bool ToYAMLValue(YAMLValue& output, const YAMLValueConversionContext& context = {});
    bool FromYAMLValue(const YAMLValue& input, const YAMLValueConversionContext& context = {});

    Transform* transform() const;
    GameObject* GetOwner() const { return m_owner; }

    template <typename T>
    T* GetComponent() const;

protected:
    virtual void RegisterFields(FieldRegistry& fields) {}
};
