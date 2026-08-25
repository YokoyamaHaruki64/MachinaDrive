#include "pch.h"

#include "System/Core/GameObjectReference.h"

#include "System/Core/GameObject.h"

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
    if (other.m_isResolutionError)
    {
        MarkResolutionError();
    }
    else
    {
        Set(other.m_target);
    }
}

GameObjectReference& GameObjectReference::operator=(const GameObjectReference& other)
{
    if (other.m_isResolutionError)
    {
        MarkResolutionError();
    }
    else
    {
        Set(other.m_target);
    }
    return *this;
}

GameObjectReference::GameObjectReference(GameObjectReference&& other) noexcept
{
    if (other.m_isResolutionError)
    {
        MarkResolutionError();
    }
    else
    {
        Set(other.m_target);
    }
    other.Clear();
}

GameObjectReference& GameObjectReference::operator=(GameObjectReference&& other) noexcept
{
    if (this != &other)
    {
        if (other.m_isResolutionError)
        {
            MarkResolutionError();
        }
        else
        {
            Set(other.m_target);
        }
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

bool GameObjectReference::IsResolutionError() const
{
    return m_isResolutionError;
}

void GameObjectReference::Set(GameObject* target)
{
    m_isResolutionError = false;

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
    m_isResolutionError = false;

    GameObject* target = std::exchange(m_target, nullptr);
    if (target)
    {
        target->UnregisterReference(this);
    }
}

void GameObjectReference::MarkResolutionError()
{
    GameObject* target = std::exchange(m_target, nullptr);
    if (target)
    {
        target->UnregisterReference(this);
    }

    m_isResolutionError = true;
}
