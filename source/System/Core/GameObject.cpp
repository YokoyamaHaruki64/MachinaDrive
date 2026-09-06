#include "pch.h"

#include "System/Core/GameObject.h"
#include <algorithm>

#include "System/Core/GameObjectReference.h"
#include "Component/Transform.h"
#include "Component/TransformBase.h"
#include "Component/UITransform.h"
#include "System/Core/Physics.h"

GameObject::GameObject()
    : GameObject("Empty", Vector3::Zero(), Vector3::Zero(), Vector3::One())
{
}

GameObject::GameObject(const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : GameObject("Empty", position, rotation, scale)
{
}

GameObject::GameObject(
    std::string name,
    const Vector3& position,
    const Vector3& rotation,
    const Vector3& scale,
    GameObjectTransformType transformType)
    : m_name(std::move(name))
{
    if (transformType == GameObjectTransformType::UI)
    {
        AddComponent<UITransform>();
    }
    else
    {
        AddComponent<Transform>(position, rotation, scale);
    }
}

GameObject::~GameObject()
{
}

void GameObject::SetActive(bool active)
{
    if (m_isActive == active)
    {
        return;
    }

    const bool notifyLifecycle = m_isStarted && !m_isDestroyRequested && !m_isDestroyed;
    m_isActive = active;
    Physics::NotifyGameObjectStateChanged(this);

    if (!notifyLifecycle)
    {
        return;
    }

    if (m_isActive)
    {
        NotifyActiveState();
    }
    else
    {
        NotifyInactiveState();
    }
}

Transform* GameObject::GetTransform() const
{
    return dynamic_cast<Transform*>(m_transformBase);
}

UITransform* GameObject::GetUITransform() const
{
    return dynamic_cast<UITransform*>(m_transformBase);
}

GameObject* GameObject::GetParent() const
{
    TransformBase* const parent = m_transformBase ? m_transformBase->GetParent() : nullptr;
    return parent ? parent->GetOwner() : nullptr;
}

void GameObject::Destroy()
{
    if (m_isDestroyed || m_isDestroyRequested)
    {
        return;
    }

    ClearReferences();
    if (m_transformBase)
    {
        m_transformBase->DestroyChildren();
    }
    m_isDestroyRequested = true;
    Physics::NotifyGameObjectStateChanged(this);
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
    return m_transformBase && m_transformBase->SetParent(
        parent ? parent->m_transformBase : nullptr,
        mode);
}

bool GameObject::SetParent(GameObject* parent)
{
    return SetParent(parent, ParentTransformMode::KeepWorld);
}
