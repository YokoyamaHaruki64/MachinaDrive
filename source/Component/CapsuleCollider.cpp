#include "pch.h"

#include "Component/CapsuleCollider.h"

#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"

void CapsuleCollider::RegisterFields(FieldRegistry& fields)
{
    Collider::RegisterFields(fields);
    GM_FIELD_ACCESSOR_TYPE(float, "m_radius", GetRadius(), SetRadius(value), All, InspectorFieldType::Auto, true);
    GM_FIELD_ACCESSOR_TYPE(float, "m_height", GetHeight(), SetHeight(value), All, InspectorFieldType::Auto, true);
}

void CapsuleCollider::SetRadius(float radius)
{
    // 半径を変更しても、両端の半球が潰れないよう全高の下限を同時に引き上げる。
    m_radius = std::max(radius, 0.0f);
    m_height = std::max(m_height, m_radius * 2.0f);
    Physics::MarkBodyShapeDirty(this);
}

void CapsuleCollider::SetHeight(float height)
{
    // Capsuleの最小高さは上下の半球が接する直径。これ未満なら球として扱われるため、
    // 設定値を丸めて常に「中心線 + 半径」の表現を成立させる。
    m_height = std::max(height, m_radius * 2.0f);
    Physics::MarkBodyShapeDirty(this);
}
