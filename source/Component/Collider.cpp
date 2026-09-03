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

void Collider::OnEnable()
{
    Physics::RegisterCollider(this);
}

void Collider::OnDisable()
{
    Physics::UnregisterCollider(this);
}

void Collider::OnDestroy()
{
    Physics::UnregisterCollider(this);
}

void Collider::OnTransformChanged(const TransformChangedEvent& event)
{
    if (HasTransformChange(event.changes, TransformChange::Hierarchy))
    {
        // 親変更によるRigidbody所属の変化はCollider自身が解釈してBackendへ伝える。
        Physics::RebindCollider(this);
        return;
    }

    Rigidbody* rigidbody = GetRigidbody();
    if (!rigidbody)
    {
        if (HasTransformChange(event.changes, TransformChange::Scale))
        {
            Physics::MarkBodyShapeDirty(this);
        }
        else
        {
            Physics::MarkBodyTransformDirty(this);
        }
        return;
    }

    GameObject* rigidbodyOwner = rigidbody->GetOwner();
    GameObject* sourceOwner = event.source ? event.source->GetOwner() : nullptr;
    bool sourceIsInsideBody = false;
    for (GameObject* object = sourceOwner; object; object = object->GetParent())
    {
        if (object == rigidbodyOwner)
        {
            sourceIsInsideBody = true;
            break;
        }
    }

    if (HasTransformChange(event.changes, TransformChange::Scale)
        || (sourceIsInsideBody && sourceOwner != rigidbodyOwner))
    {
        // Body rootより下の相対姿勢、またはWorld Scaleの変化だけがCompound Shapeへ影響する。
        Physics::MarkBodyShapeDirty(this);
    }
}

void Collider::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD_ACCESSOR(Vector3, "m_center", GetCenter(), SetCenter(value), All);
    GM_FIELD_ACCESSOR(bool, "m_isTrigger", IsTrigger(), SetIsTrigger(value), All);
    GM_FIELD_ACCESSOR(int, "m_layer", GetLayer(), SetLayer(value), All);
}

void Collider::SetCenter(const Vector3& center)
{
    m_center = center;
    Physics::MarkBodyShapeDirty(this);
}

void Collider::SetIsTrigger(bool isTrigger)
{
    m_isTrigger = isTrigger;
    Physics::MarkBodyShapeDirty(this);
}

void Collider::SetLayer(int layer)
{
    m_layer = layer;
    Physics::MarkBodyShapeDirty(this);
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
