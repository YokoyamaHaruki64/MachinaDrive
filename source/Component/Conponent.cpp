#include "pch.h"

#include "Component/ComponentInc.h"



Transform* Component::transform() const
{
    return m_owner ? m_owner->GetTransform() : nullptr;
}
