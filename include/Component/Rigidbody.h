#pragma once

#include "System/Core/ComponentRegistry.h"
#include "System/Core/component.h"
#include "math/Vector3.h"

// Rigidbodyは速度と衝突解決に必要な物理値だけを保持する。
// 速度の積分や重力は呼び出し側のScriptで必要に応じて行う。
// Massが0のRigidbody、またはRigidbodyを持たないObjectは静的物体として扱う。
// 判定解決では質量の逆数を使うため、静的物体の逆質量を0にすると、動的側だけを動かせる。
class Rigidbody : public Component
{
    REGISTER_COMPONENT(Rigidbody, "Rigidbody");

    float m_mass{ 1.0f };
    Vector3 m_velocity{ Vector3::Zero() };
    float m_bounciness{ 0.0f };

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    Rigidbody() = delete;
    explicit Rigidbody(GameObject* owner)
        : Component(owner)
    {
    }

    float GetMass() const { return m_mass; }
    void SetMass(float mass);

    float GetInverseMass() const { return m_mass > 0.0f ? 1.0f / m_mass : 0.0f; }

    const Vector3& GetVelocity() const { return m_velocity; }
    void SetVelocity(const Vector3& velocity) { m_velocity = velocity; }
    void AddVelocity(const Vector3& velocity) { m_velocity += velocity; }

    float GetBounciness() const { return m_bounciness; }
    void SetBounciness(float bounciness);
};
