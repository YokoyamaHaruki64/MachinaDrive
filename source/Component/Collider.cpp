#include "pch.h"

#include "Component/Collider.h"

#include "Component/Rigidbody.h"
#include "Component/Transform.h"

void Collider::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_center);
}

Vector3 Collider::GetWorldCenter() const
{
    Transform* ownerTransform = transform();
    if (!ownerTransform)
    {
        // 生成直後などでTransformがまだ取得できない場合はLocal値をそのまま返す。
        return m_center;
    }

    // OffsetにもObjectのScaleを適用してからRotationで回す。
    // これにより、Collider中心をObject原点からずらした場合もTransformと同じ座標系になる。
    const Vector3 scaledCenter = Vector3::Multiply(m_center, ownerTransform->GetScale());
    return ownerTransform->GetPosition()
        + ownerTransform->GetRotation().RotateVector(scaledCenter);
}

Rigidbody* Collider::GetRigidbody() const
{
    return m_owner ? m_owner->GetComponent<Rigidbody>() : nullptr;
}
