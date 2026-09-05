#include "pch.h"

#include "Component/SphereCollider.h"

#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"

void SphereCollider::RegisterFields(FieldRegistry& fields)
{
    Collider::RegisterFields(fields);
    GM_FIELD_ACCESSOR_TYPE(float, "m_radius", GetRadius(), SetRadius(value), All, InspectorFieldType::Auto, true);
}

void SphereCollider::SetRadius(float radius)
{
    m_radius = std::max(radius, 0.0f);
    Physics::MarkBodyShapeDirty(this);
}
