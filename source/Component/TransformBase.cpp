#include "pch.h"

#include <algorithm>

#include "Component/TransformBase.h"
#include "System/Core/GameObject.h"
#include "System/Diagnostics/Logger.h"

TransformBase::TransformBase(GameObject* owner)
    : Component(owner)
{
    if (m_owner)
    {
        m_owner->RegisterTransformBase(this);
    }
}

TransformBase::~TransformBase()
{
    if (m_parent)
    {
        m_parent->RemoveChild(this);
    }

    const std::vector<TransformBase*> children = m_children;
    for (TransformBase* child : children)
    {
        if (child && child->m_parent == this)
        {
            child->SetParent(nullptr, ParentTransformMode::KeepLocal);
        }
    }
}

void TransformBase::AddChild(TransformBase* child)
{
    const auto insertionPosition = std::lower_bound(
        m_children.begin(),
        m_children.end(),
        child,
        [](const TransformBase* left, const TransformBase* right)
        {
            return left->GetOwner()->GetId() < right->GetOwner()->GetId();
        });
    m_children.insert(insertionPosition, child);
}

void TransformBase::RemoveChild(TransformBase* child)
{
    std::erase(m_children, child);
}

bool TransformBase::WouldCreateCycle(const TransformBase* newParent) const
{
    for (const TransformBase* ancestor = newParent; ancestor; ancestor = ancestor->m_parent)
    {
        if (ancestor == this)
        {
            return true;
        }
    }
    return false;
}

bool TransformBase::SetParent(TransformBase* parent, ParentTransformMode)
{
    if (parent == m_parent)
    {
        return true;
    }

    if (parent && typeid(*this) != typeid(*parent))
    {
        Logger::Error("異なるTransform種別の親子関係は作成できません。");
        return false;
    }

    if (parent && WouldCreateCycle(parent))
    {
        Logger::Error("Transformの親子関係で循環参照は作成できません。");
        return false;
    }

    TransformBase* const previousParent = m_parent;

    if (previousParent)
    {
        previousParent->RemoveChild(this);
    }

    m_parent = parent;
    if (m_parent)
    {
        m_parent->AddChild(this);
    }

    return true;
}

void TransformBase::DestroyChildren()
{
    const std::vector<TransformBase*> children = m_children;
    for (TransformBase* child : children)
    {
        if (child && child->GetOwner())
        {
            child->GetOwner()->Destroy();
        }
    }
}
