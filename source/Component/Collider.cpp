#include "pch.h"

#include "Component/Collider.h"

#include "Component/Rigidbody.h"
#include "Component/Transform.h"
#include "System/Debug/DebugDrawer.h"

Collider::Collider(GameObject* owner)
    : Component(owner)
{
#if GM_ENABLE_EDITOR
    // 生成時に登録し、DebugDrawer側で毎フレーム現在のTransformから頂点を作り直す。
    DebugDrawer::RegisterCollider(this);
#endif
}

Collider::~Collider()
{
#if GM_ENABLE_EDITOR
    // Colliderより先にGameObjectが破棄されても、登録リストに無効なポインタを残さない。
    DebugDrawer::UnregisterCollider(this);
#endif
}

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

    // centerはCollider自身の位置オフセットであり、形状の大きさとは別に扱う。
    // そのためScaleはBoxのsizeやCapsuleのradius/heightだけへ適用し、
    // centerにはRotationとPositionだけを適用する。
    return ownerTransform->GetPosition()
        + ownerTransform->GetRotation().RotateVector(m_center);
}

Rigidbody* Collider::GetRigidbody() const
{
    return m_owner ? m_owner->GetComponent<Rigidbody>() : nullptr;
}
