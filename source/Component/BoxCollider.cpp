#include "pch.h"

#include "Component/BoxCollider.h"

#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"

void BoxCollider::RegisterFields(FieldRegistry& fields)
{
    Collider::RegisterFields(fields);
    GM_FIELD_ACCESSOR_TYPE(Vector3, "m_size", GetSize(), SetSize(value), All, InspectorFieldType::Auto, true);
}

void BoxCollider::SetSize(const Vector3& size)
{
    m_size = size;
    Physics::MarkBodyShapeDirty(this);
}
