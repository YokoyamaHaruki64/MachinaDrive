#include "pch.h"

#include "System/GameObject.h"
#include "Component/Transform.h"

GameObject::GameObject()
    : GameObject("Empty", Vector3::Zero(), Vector3::Zero(), Vector3::One())
{
}

GameObject::GameObject(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : GameObject("Empty", position, rotation, scale)
{
}

GameObject::GameObject(std::string name, const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : m_name(std::move(name))
{
    m_transform = AddComponent<Transform>(position, rotation, scale);
}

Transform* GameObject::GetTransform() const
{
    return m_transform;
}

void GameObject::Destroy()
{
    if (m_isDestroyed || m_isDestroyRequested)
    {
        return;
    }

    m_transform->DestroyChildren();
    m_isDestroyRequested = true;
}

bool GameObject::SetParent(GameObject* parent, ParentTransformMode mode)
{
    return m_transform->SetParent(parent ? parent->GetTransform() : nullptr, mode);
}

bool GameObject::SetParent(GameObject* parent)
{
    return SetParent(parent, ParentTransformMode::KeepWorld);
}
