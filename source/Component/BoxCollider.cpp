#include "pch.h"

#include "Component/BoxCollider.h"

#include "System/Editor/FieldRegistry.h"

void BoxCollider::RegisterFields(FieldRegistry& fields)
{
    Collider::RegisterFields(fields);
    GM_FIELD(m_size);
}
