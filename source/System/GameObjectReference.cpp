#include "pch.h"

#include "System/GameObjectReference.h"

#include "System/GameObject.h"

GameObjectReference::GameObjectReference(GameObject* target)
{
    Set(target);
}

GameObjectReference::~GameObjectReference()
{
    Clear();
}

GameObjectReference::GameObjectReference(const GameObjectReference& other)
{
    Set(other.m_target);
}

GameObjectReference& GameObjectReference::operator=(const GameObjectReference& other)
{
    Set(other.m_target);
    return *this;
}

GameObjectReference::GameObjectReference(GameObjectReference&& other) noexcept
{
    Set(other.m_target);
    other.Clear();
}

GameObjectReference& GameObjectReference::operator=(GameObjectReference&& other) noexcept
{
    if (this != &other)
    {
        Set(other.m_target);
        other.Clear();
    }

    return *this;
}

GameObjectReference& GameObjectReference::operator=(GameObject* target)
{
    Set(target);
    return *this;
}

GameObject* GameObjectReference::operator->() const
{
    return m_target;
}

GameObject& GameObjectReference::operator*() const
{
    return *m_target;
}

GameObjectReference::operator bool() const
{
    return m_target != nullptr;
}

GameObject* GameObjectReference::Get() const
{
    return m_target;
}

void GameObjectReference::Set(GameObject* target)
{
    if (target && target->IsDestroyed())
    {
        target = nullptr;
    }

    if (m_target == target)
    {
        return;
    }

    Clear();

    m_target = target;
    if (m_target)
    {
        m_target->RegisterReference(this);
    }
}

void GameObjectReference::Clear()
{
    GameObject* target = std::exchange(m_target, nullptr);
    if (target)
    {
        target->UnregisterReference(this);
    }
}
