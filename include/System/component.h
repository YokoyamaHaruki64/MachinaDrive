#pragma once

#include "math/Vector3.h"

class Transform;
class GameObject;
class FieldRegistry;

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

    void DrawInspectorImGui();

    Transform* transform() const;

    template <typename T>
    T* GetComponent() const;

protected:
    virtual void RegisterFields(FieldRegistry& fields) {}
};
