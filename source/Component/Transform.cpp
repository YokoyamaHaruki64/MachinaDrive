#include "pch.h"

#include <algorithm>

#include "Component/Transform.h"
#include "System/Editor/FieldRegistry.h"
#include "System/Core/GameObject.h"
#include "System/Diagnostics/Logger.h"
#include "math/MathCommon.h"

Transform::Transform(GameObject* owner, const Vector3& position, const Vector3& rotation, const Vector3& scale)
    : Component(owner),
    m_localPosition(position),
    m_localRotation(Quaternion::EulerToQuat(rotation)),
    m_localScale(scale)
{
}

Transform::~Transform()
{
    SetParent(nullptr, ParentTransformMode::KeepLocal);

    for (Transform* child : m_children)
    {
        child->m_parent = nullptr;
        child->MarkWorldDirty();
    }
}

void Transform::MarkWorldDirty()
{
    if (m_isWorldDirty)
    {
        return;
    }

    m_isWorldDirty = true;
    for (Transform* child : m_children)
    {
        child->MarkWorldDirty();
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
    m_worldMatrix = m_parent ? localMatrix * m_parent->GetWorldMatrix() : localMatrix;

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
    if (!m_parent)
    {
        SetLocalFromMatrix(worldMatrix);
        return;
    }

    const XMMATRIX parentInverse = XMMatrixInverse(nullptr, m_parent->GetWorldMatrix().ToXMMATRIX());
    SetLocalFromMatrix(Matrix4x4::FromXMMATRIX(worldMatrix.ToXMMATRIX() * parentInverse));
}

void Transform::AddChild(Transform* child)
{
    m_children.push_back(child);
}

void Transform::RemoveChild(Transform* child)
{
    std::erase(m_children, child);
}

bool Transform::WouldCreateCycle(const Transform* newParent) const
{
    for (const Transform* ancestor = newParent; ancestor; ancestor = ancestor->m_parent)
    {
        if (ancestor == this)
        {
            return true;
        }
    }
    return false;
}

bool Transform::SetParent(Transform* parent, ParentTransformMode mode)
{
    if (parent == m_parent)
    {
        return true;
    }

    if (parent && WouldCreateCycle(parent))
    {
        Logger::Error("Transformの親子関係で循環参照は作成できません。");
        return false;
    }

    const Matrix4x4 previousWorldMatrix = GetWorldMatrix();

    if (m_parent)
    {
        m_parent->RemoveChild(this);
    }

    m_parent = parent;
    if (m_parent)
    {
        m_parent->AddChild(this);
    }

    if (mode == ParentTransformMode::KeepWorld)
    {
        SetWorldMatrix(previousWorldMatrix);
    }
    else
    {
        MarkWorldDirty();
    }
    return true;
}

void Transform::DestroyChildren()
{
    const std::vector<Transform*> children = m_children;
    for (Transform* child : children)
    {
        child->m_owner->Destroy();
    }
}

void Transform::SetPosition(const Vector3& position)
{
    UpdateWorldCache();
    SetWorldMatrix(Matrix4x4::Scaling(m_worldScale.x, m_worldScale.y, m_worldScale.z)
        * m_worldRotation.ToMatrix()
        * Matrix4x4::Translation(position.x, position.y, position.z));
}

void Transform::SetRotation(const Quaternion& rotation)
{
    UpdateWorldCache();
    SetWorldMatrix(Matrix4x4::Scaling(m_worldScale.x, m_worldScale.y, m_worldScale.z)
        * rotation.Normalized().ToMatrix()
        * Matrix4x4::Translation(m_worldPosition.x, m_worldPosition.y, m_worldPosition.z));
}

void Transform::SetScale(const Vector3& scale)
{
    UpdateWorldCache();
    SetWorldMatrix(Matrix4x4::Scaling(scale.x, scale.y, scale.z)
        * m_worldRotation.ToMatrix()
        * Matrix4x4::Translation(m_worldPosition.x, m_worldPosition.y, m_worldPosition.z));
}

void Transform::SetLocalPosition(const Vector3& position)
{
    m_localPosition = position;
    MarkWorldDirty();
}

void Transform::SetLocalRotation(const Quaternion& rotation)
{
    m_localRotation = rotation.Normalized();
    MarkWorldDirty();
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
