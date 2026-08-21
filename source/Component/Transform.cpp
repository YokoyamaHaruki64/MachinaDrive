#include "pch.h"

#include "Component/Transform.h"
#include "System/FieldRegistry.h"

void Transform::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_position);
    GM_FIELD(m_rotation);
    GM_FIELD(m_scale);
}
