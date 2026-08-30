#pragma once

#include <typeinfo>
#include <vector>

#include "System/Core/component.h"

enum class ParentTransformMode
{
    KeepWorld,
    KeepLocal,
};

class TransformBase : public Component
{
protected:
    TransformBase* m_parent = nullptr;
    std::vector<TransformBase*> m_children;

    void AddChild(TransformBase* child);
    void RemoveChild(TransformBase* child);
    bool WouldCreateCycle(const TransformBase* newParent) const;

public:
    TransformBase() = delete;
    explicit TransformBase(GameObject* owner);
    ~TransformBase() override;

    TransformBase(const TransformBase&) = delete;
    TransformBase& operator=(const TransformBase&) = delete;

    virtual bool SetParent(TransformBase* parent, ParentTransformMode mode = ParentTransformMode::KeepWorld);
    TransformBase* GetParent() const { return m_parent; }
    const std::vector<TransformBase*>& GetChildren() const { return m_children; }
    void DestroyChildren();
};
