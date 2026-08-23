#include "pch.h"

#include "System/GameObject.h"
#include "System/GameObjectReference.h"
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

    ClearReferences();
    m_transform->DestroyChildren();
    m_isDestroyRequested = true;
}

void GameObject::RegisterReference(GameObjectReference* reference)
{
    m_references.push_back(reference);
}

void GameObject::UnregisterReference(GameObjectReference* reference)
{
    const auto iterator = std::find(m_references.begin(), m_references.end(), reference);
    if (iterator != m_references.end())
    {
        m_references.erase(iterator);
    }
}

void GameObject::ClearReferences()
{
    // Clear側で登録解除されるため、末尾から処理してイテレータを保持しない。
    while (!m_references.empty())
    {
        m_references.back()->Clear();
    }
}

bool GameObject::SetParent(GameObject* parent, ParentTransformMode mode)
{
    return m_transform->SetParent(parent ? parent->GetTransform() : nullptr, mode);
}

bool GameObject::SetParent(GameObject* parent)
{
    return SetParent(parent, ParentTransformMode::KeepWorld);
}
