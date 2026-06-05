#include "System/component.h"
#include "System/GameObject.h"

Transform* Component::transform() const
{
    return m_owner ? m_owner->GetTransform() : nullptr;
}
