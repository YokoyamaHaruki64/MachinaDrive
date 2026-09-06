#include "pch.h"

#include "Component/Transform.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Core/GameObject.h"
#include "System/Diagnostics/Logger.h"
#include "math/MathCommon.h"

Transform::Transform(GameObject* owner, const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : TransformBase(owner),
    m_localPosition(position),
    m_localRotation(Quaternion::EulerToQuat(rotation)),
    m_localScale(scale)
{
}

void Transform::MarkWorldDirty()
{
    if (m_isWorldDirty)
    {
        return;
    }

    m_isWorldDirty = true;
    for (TransformBase* child : m_children)
    {
        if (Transform* childTransform = dynamic_cast<Transform*>(child))
        {
            childTransform->MarkWorldDirty();
        }
    }
}

void Transform::NotifyTransformChanged(Transform* source, TransformChange changes)
{
    if (m_owner)
    {
        m_owner->NotifyTransformChanged({ source, changes });
    }

    for (TransformBase* child : m_children)
    {
        if (Transform* childTransform = dynamic_cast<Transform*>(child))
        {
            childTransform->NotifyTransformChanged(source, changes);
        }
    }
}

void Transform::UpdateWorldCache() const
{
    if (!m_isWorldDirty)
    {
        return;
    }

    const Matrix4x4 localMatrix = Matrix4x4::Scaling(m_localScale.x, m_localScale.y, m_localScale.z)
        * m_localRotation.ToMatrix()
        * Matrix4x4::Translation(m_localPosition.x, m_localPosition.y, m_localPosition.z);
    const Transform* parent = dynamic_cast<const Transform*>(m_parent);
    m_worldMatrix = parent ? localMatrix * parent->GetWorldMatrix() : localMatrix;

    XMVECTOR scale;
    XMVECTOR rotation;
    XMVECTOR translation;
    if (XMMatrixDecompose(&scale, &rotation, &translation, m_worldMatrix.ToXMMATRIX()))
    {
        XMFLOAT3 decomposedScale;
        XMFLOAT3 decomposedPosition;
        XMStoreFloat3(&decomposedScale, scale);
        XMStoreFloat3(&decomposedPosition, translation);

        m_worldScale = Vector3(decomposedScale.x, decomposedScale.y, decomposedScale.z);
        m_worldRotation = Quaternion(rotation.m128_f32[0], rotation.m128_f32[1], rotation.m128_f32[2], rotation.m128_f32[3]);
        m_worldPosition = Vector3(decomposedPosition.x, decomposedPosition.y, decomposedPosition.z);
    }
    else
    {
        // 非可逆なScaleでも位置だけは常に取り出せるようにする。
        m_worldPosition = Vector3(m_worldMatrix.m[3][0], m_worldMatrix.m[3][1], m_worldMatrix.m[3][2]);
        m_worldRotation = Quaternion::Identity();
        m_worldScale = Vector3::Zero();
    }

    m_isWorldDirty = false;
}

void Transform::SetLocalFromMatrix(const Matrix4x4& localMatrix)
{
    XMVECTOR scale;
    XMVECTOR rotation;
    XMVECTOR translation;
    if (!XMMatrixDecompose(&scale, &rotation, &translation, localMatrix.ToXMMATRIX()))
    {
        Logger::Error("Transformの行列をLocal TRSへ分解できません。");
        return;
    }

    XMFLOAT3 decomposedScale;
    XMFLOAT3 decomposedPosition;
    XMStoreFloat3(&decomposedScale, scale);
    XMStoreFloat3(&decomposedPosition, translation);

    m_localScale = Vector3(decomposedScale.x, decomposedScale.y, decomposedScale.z);
    m_localRotation = Quaternion(rotation.m128_f32[0], rotation.m128_f32[1], rotation.m128_f32[2], rotation.m128_f32[3]);
    m_localPosition = Vector3(decomposedPosition.x, decomposedPosition.y, decomposedPosition.z);
    MarkWorldDirty();
}

void Transform::SetWorldMatrix(const Matrix4x4& worldMatrix)
{
    const Transform* parent = dynamic_cast<const Transform*>(m_parent);
    if (!parent)
    {
        SetLocalFromMatrix(worldMatrix);
        return;
    }

    const XMMATRIX parentInverse = XMMatrixInverse(nullptr, parent->GetWorldMatrix().ToXMMATRIX());
    SetLocalFromMatrix(Matrix4x4::FromXMMATRIX(worldMatrix.ToXMMATRIX() * parentInverse));
}

bool Transform::SetParent(TransformBase* parent, ParentTransformMode mode)
{
    if (parent == m_parent)
    {
        return true;
    }

    const Matrix4x4 previousWorldMatrix = mode == ParentTransformMode::KeepWorld
        ? GetWorldMatrix()
        : Matrix4x4::Identity();
    if (!TransformBase::SetParent(parent, ParentTransformMode::KeepLocal))
    {
        return false;
    }

    if (mode == ParentTransformMode::KeepWorld)
    {
        SetWorldMatrix(previousWorldMatrix);
    }
    else
    {
        MarkWorldDirty();
    }

    NotifyTransformChanged(this, TransformChange::Hierarchy);
    return true;
}

void Transform::SetPosition(const Vector3& position)
{
    UpdateWorldCache();
    SetWorldMatrix(Matrix4x4::Scaling(m_worldScale.x, m_worldScale.y, m_worldScale.z)
        * m_worldRotation.ToMatrix()
        * Matrix4x4::Translation(position.x, position.y, position.z));
    NotifyTransformChanged(this, TransformChange::Position);
}

void Transform::SetRotation(const Quaternion& rotation)
{
    UpdateWorldCache();
    SetWorldMatrix(Matrix4x4::Scaling(m_worldScale.x, m_worldScale.y, m_worldScale.z)
        * rotation.Normalized().ToMatrix()
        * Matrix4x4::Translation(m_worldPosition.x, m_worldPosition.y, m_worldPosition.z));
    NotifyTransformChanged(this, TransformChange::Rotation);
}

void Transform::SetScale(const Vector3& scale)
{
    UpdateWorldCache();
    SetWorldMatrix(Matrix4x4::Scaling(scale.x, scale.y, scale.z)
        * m_worldRotation.ToMatrix()
        * Matrix4x4::Translation(m_worldPosition.x, m_worldPosition.y, m_worldPosition.z));
    NotifyTransformChanged(this, TransformChange::Scale);
}

void Transform::SetLocalPosition(const Vector3& position)
{
    m_localPosition = position;
    MarkWorldDirty();
    NotifyTransformChanged(this, TransformChange::Position);
}

void Transform::SetLocalRotation(const Quaternion& rotation)
{
    m_localRotation = rotation.Normalized();
    MarkWorldDirty();
    NotifyTransformChanged(this, TransformChange::Rotation);
}

void Transform::SetEulerAngles(const Vector3& eulerRadians)
{
    SetRotation(Quaternion::EulerToQuat(eulerRadians));
}

void Transform::SetLocalEulerAngles(const Vector3& eulerRadians)
{
    SetLocalRotation(Quaternion::EulerToQuat(eulerRadians));
}

void Transform::SetLocalScale(const Vector3& scale)
{
    m_localScale = scale;
    MarkWorldDirty();
    NotifyTransformChanged(this, TransformChange::Scale);
}

const Matrix4x4& Transform::GetWorldMatrix() const
{
    UpdateWorldCache();
    return m_worldMatrix;
}

const Vector3& Transform::GetPosition() const
{
    UpdateWorldCache();
    return m_worldPosition;
}

const Quaternion& Transform::GetRotation() const
{
    UpdateWorldCache();
    return m_worldRotation;
}

const Vector3& Transform::GetScale() const
{
    UpdateWorldCache();
    return m_worldScale;
}

void Transform::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD_ACCESSOR(Vector3, "position", GetPosition(), SetPosition(value), World);
    GM_FIELD_ACCESSOR(Vector3, "rotation (deg)", GetEulerAngles() * Math::RAD2DEG, SetEulerAngles(value * Math::DEG2RAD), World);
    GM_FIELD_ACCESSOR(Vector3, "scale", GetScale(), SetScale(value), World);

    GM_FIELD_ACCESSOR(Vector3, "position", GetLocalPosition(), SetLocalPosition(value), Local);
    GM_FIELD_ACCESSOR(Vector3, "rotation (deg)", GetLocalEulerAngles() * Math::RAD2DEG, SetLocalEulerAngles(value * Math::DEG2RAD), Local);
    GM_FIELD_ACCESSOR(Vector3, "scale", GetLocalScale(), SetLocalScale(value), Local);

    // Inspector表示とは独立したキーで、Transformの正であるLocal TRSを保存する。
    GM_FIELD_ACCESSOR_TYPE(Vector3, "m_localPosition", GetLocalPosition(), SetLocalPosition(value), All, InspectorFieldType::Hidden, true);
    GM_FIELD_ACCESSOR_TYPE(Quaternion, "m_localRotation", GetLocalRotation(), SetLocalRotation(value), All, InspectorFieldType::Hidden, true);
    GM_FIELD_ACCESSOR_TYPE(Vector3, "m_localScale", GetLocalScale(), SetLocalScale(value), All, InspectorFieldType::Hidden, true);
}
