#include "pch.h"

#include "System/GameObject.h"
#include "Component/Transform.h"

GameObject::GameObject()
{
    m_transform = AddComponent<Transform>(Vector3::Zero(), Vector3::Zero(), Vector3::One());
}

GameObject::GameObject(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    m_transform = AddComponent<Transform>(position, rotation, scale);
}

Transform* GameObject::GetTransform() const
{
    return m_transform;
}
