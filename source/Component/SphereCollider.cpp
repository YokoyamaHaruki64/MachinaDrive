#include "pch.h"

#include "Component/SphereCollider.h"

#include "System/Editor/FieldRegistry.h"

void SphereCollider::RegisterFields(FieldRegistry& fields)
{
    Collider::RegisterFields(fields);
    GM_FIELD(m_radius);
}

void SphereCollider::SetRadius(float radius)
{
    m_radius = std::max(radius, 0.0f);
}
