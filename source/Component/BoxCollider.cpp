#include "pch.h"

#include "Component/BoxCollider.h"

#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"

void BoxCollider::RegisterFields(FieldRegistry& fields)
{
    Collider::RegisterFields(fields);
    GM_FIELD(m_size);
}

void BoxCollider::SetSize(const Vector3& size)
{
    m_size = size;
    Physics::MarkDirty();
}
