#pragma once

#include "System/main.h"

#include "System/Component.h"
#include "math/Vector3.h"

class Transform : public Component
{
    Vector3 m_position{ 0.0f, 0.0f, 0.0f };
    Vector3 m_rotation{ 0.0f, 0.0f, 0.0f };
    Vector3 m_scale{ 1.0f, 1.0f, 1.0f };

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    Transform() = delete;
    Transform(GameObject* owner) : Component(owner) {}
    Transform(GameObject* owner, const Vector3& position, const Vector3& rotation, const Vector3& scale)
        : Component(owner), m_position(position), m_rotation(rotation), m_scale(scale)
    {
    }

    Transform(const Transform&) = delete;
    Transform& operator=(const Transform&) = delete;


    ~Transform() {}

    void SetPosition(const Vector3& position) { m_position = position; }
    void SetRotation(const Vector3& rotation) { m_rotation = rotation; }
    void SetScale(const Vector3& scale) { m_scale = scale; }

    void Translate(const Vector3& delta) { m_position += delta; }
    void Rotate(const Vector3& delta) { m_rotation += delta; }
    void Scale(const Vector3& delta) { m_scale += delta; }

    const Vector3& GetPosition() const { return m_position; }
    const Vector3& GetRotation() const { return m_rotation; }
    const Vector3& GetScale() const { return m_scale; }

    const Vector3 GetForward() const
    {
        XMMATRIX rotationMatrix =
            XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
        Vector3 forward;
        XMStoreFloat3((XMFLOAT3*)&forward, rotationMatrix.r[2]);
        return forward;
    }

    const Vector3 GetRight() const
    {
        XMMATRIX rotationMatrix =
            XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
        Vector3 right;
        XMStoreFloat3((XMFLOAT3*)&right, rotationMatrix.r[0]);
        return right;
    }

    const Vector3 GetUp() const
    {
        XMMATRIX rotationMatrix =
            XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);
        Vector3 up;
        XMStoreFloat3((XMFLOAT3*)&up, rotationMatrix.r[1]);
        return up;
    }

};
