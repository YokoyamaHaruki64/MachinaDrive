#pragma once

#include "System/Core/main.h"

#include "System/Core/ComponentRegistry.h"
#include "Component/TransformBase.h"
#include "math/Matrix4x4.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

class Transform : public TransformBase
{
    REGISTER_COMPONENT_OPTIONS(Transform, "Transform", false, false);

    Vector3 m_localPosition{ Vector3::Zero() };
    Quaternion m_localRotation{ Quaternion::Identity() };
    Vector3 m_localScale{ Vector3::One() };

    mutable bool m_isWorldDirty = true;
    mutable Matrix4x4 m_worldMatrix{ Matrix4x4::Identity() };
    mutable Vector3 m_worldPosition{ Vector3::Zero() };
    mutable Quaternion m_worldRotation{ Quaternion::Identity() };
    mutable Vector3 m_worldScale{ Vector3::One() };
    void MarkWorldDirty();
    void UpdateWorldCache() const;
    void SetLocalFromMatrix(const Matrix4x4& localMatrix);
    void SetWorldMatrix(const Matrix4x4& worldMatrix);

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    Transform() = delete;
    explicit Transform(GameObject* owner) : TransformBase(owner) {}
    Transform(GameObject* owner, const Vector3& position, const Vector3& rotation, const Vector3& scale);

    Transform(const Transform&) = delete;
    Transform& operator=(const Transform&) = delete;

    bool SetParent(TransformBase* parent, ParentTransformMode mode = ParentTransformMode::KeepWorld) override;
    void SetPosition(const Vector3& position);
    void SetRotation(const Quaternion& rotation);
    // 既存コードのEuler入力を段階的に移行するために維持する。
    void SetRotation(const Vector3& eulerRadians) { SetEulerAngles(eulerRadians); }
    void SetScale(const Vector3& scale);

    void SetEulerAngles(const Vector3& eulerRadians);

    void Translate(const Vector3& delta) { SetPosition(GetPosition() + delta); }
    void Rotate(const Vector3& eulerDelta)
    {
        // 増分回転でEulerへ戻すとジンバルロック境界で表現が反転するため、Quaternionのまま合成する。
        SetRotation(GetRotation() * Quaternion::EulerToQuat(eulerDelta));
    }
    void Scale(const Vector3& delta) { SetScale(GetScale() + delta); }

    void SetLocalPosition(const Vector3& position);
    void SetLocalRotation(const Quaternion& rotation);
    void SetLocalScale(const Vector3& scale);
    void SetLocalEulerAngles(const Vector3& eulerRadians);
    void TranslateLocal(const Vector3& delta) { SetLocalPosition(m_localPosition + delta); }
    void RotateLocal(const Vector3& eulerDelta) { SetLocalRotation(m_localRotation * Quaternion::EulerToQuat(eulerDelta)); }
    void ScaleLocal(const Vector3& delta) { SetLocalScale(m_localScale + delta); }

    const Matrix4x4& GetWorldMatrix() const;
    const Vector3& GetPosition() const;
    const Quaternion& GetRotation() const;
    const Vector3& GetScale() const;
    Vector3 GetEulerAngles() const { return GetRotation().QuatToEuler(); }

    const Vector3& GetLocalPosition() const { return m_localPosition; }
    const Quaternion& GetLocalRotation() const { return m_localRotation; }
    const Vector3& GetLocalScale() const { return m_localScale; }
    Vector3 GetLocalEulerAngles() const { return m_localRotation.QuatToEuler(); }

    Vector3 GetForward() const { return GetRotation().RotateVector(Vector3::Forward()); }
    Vector3 GetRight() const { return GetRotation().RotateVector(Vector3::Right()); }
    Vector3 GetUp() const { return GetRotation().RotateVector(Vector3::Up()); }
};
