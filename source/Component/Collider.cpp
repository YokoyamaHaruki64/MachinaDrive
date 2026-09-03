#include "pch.h"

#include "Component/Collider.h"

#include "Component/Rigidbody.h"
#include "Component/Transform.h"
#include "System/Debug/DebugDrawer.h"
#include "System/Core/Physics.h"

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
    // 通常はOnDestroyで解除されるが、未開始のObjectが直接破棄された場合にも
    // Physics側へ無効なColliderポインタを残さない。
    Physics::UnregisterCollider(this);

#if GM_ENABLE_EDITOR
    // Colliderより先にGameObjectが破棄されても、登録リストに無効なポインタを残さない。
    DebugDrawer::UnregisterCollider(this);
#endif
}

void Collider::OnLoaded()
{
    // Parent設定やInspectorからの初期値反映が終わった後、Physicsの登録対象へ加える。
    Physics::RegisterCollider(this);
}

void Collider::OnDestroy()
{
    Physics::UnregisterCollider(this);
}

void Collider::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_center);
    GM_FIELD(m_isTrigger);
    GM_FIELD(m_layer);
}

void Collider::SetCenter(const Vector3& center)
{
    m_center = center;
    Physics::MarkDirty();
}

void Collider::SetIsTrigger(bool isTrigger)
{
    m_isTrigger = isTrigger;
    Physics::MarkDirty();
}

void Collider::SetLayer(int layer)
{
    m_layer = layer;
    Physics::MarkDirty();
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
    // Colliderは自身のObjectから親方向へ辿り、最初のRigidbodyを所属Bodyとする。
    // これにより親Rigidbodyと子Colliderを1つの物理Bodyへまとめられる。
    GameObject* owner = m_owner;
    while (owner)
    {
        if (Rigidbody* rigidbody = owner->GetComponent<Rigidbody>())
        {
            return rigidbody;
        }
        owner = owner->GetParent();
    }
    return nullptr;
}
