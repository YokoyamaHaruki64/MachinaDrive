#pragma once

#include "System/Core/ComponentRegistry.h"
#include "System/Core/component.h"
#include "math/Vector3.h"

// Rigidbodyは物理Bodyの設定値と速度を保持する。
// 速度の積分、重力、衝突解決、SleepはPhysics(Jolt)が担当し、Scriptは速度を操作する。
// RigidbodyがないColliderはStatic Body、ColliderがないRigidbodyはEmptyShape Bodyとして扱う。
class Rigidbody : public Component
{
    REGISTER_COMPONENT(Rigidbody, "Rigidbody");

public:
    enum class MotionType
    {
        Static,
        Kinematic,
        Dynamic,
    };

private:
    int m_motionType{ static_cast<int>(MotionType::Dynamic) };

    float m_mass{ 1.0f };
    Vector3 m_velocity{ Vector3::Zero() };
    float m_bounciness{ 0.0f };
    float m_friction{ 0.2f };
    bool m_useGravity{ true };
    bool m_allowSleep{ true };
    bool m_freezeRotation{ true };

    void OnTransformChanged(const TransformChangedEvent& event) override;

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    Rigidbody() = delete;
    explicit Rigidbody(GameObject* owner)
        : Component(owner)
    {
    }

    ~Rigidbody() override;

    void OnEnable() override;
    void OnDisable() override;
    void OnDestroy() override;

    MotionType GetMotionType() const
    {
        // YAMLやInspectorから不正値が入っても、Joltへ未知のMotionTypeを渡さない。
        if (m_motionType <= static_cast<int>(MotionType::Static))
        {
            return MotionType::Static;
        }
        if (m_motionType == static_cast<int>(MotionType::Kinematic))
        {
            return MotionType::Kinematic;
        }
        return MotionType::Dynamic;
    }

    void SetMotionType(MotionType motionType);

    int GetMotionTypeValue() const { return m_motionType; }
    void SetMotionTypeValue(int motionType);

    float GetMass() const { return m_mass; }
    void SetMass(float mass);

    float GetInverseMass() const { return m_mass > 0.0f ? 1.0f / m_mass : 0.0f; }

    const Vector3& GetVelocity() const { return m_velocity; }
    void SetVelocity(const Vector3& velocity);
    void AddVelocity(const Vector3& velocity);

    // Joltの解決結果を書き戻す経路。Script向けSetterのdirty通知を発生させない。
    void SetVelocityFromPhysics(const Vector3& velocity) { m_velocity = velocity; }

    float GetBounciness() const { return m_bounciness; }
    void SetBounciness(float bounciness);

    float GetFriction() const { return m_friction; }
    void SetFriction(float friction);

    bool IsUseGravity() const { return m_useGravity; }
    void SetUseGravity(bool useGravity);

    bool IsAllowSleep() const { return m_allowSleep; }
    void SetAllowSleep(bool allowSleep);

    bool IsFreezeRotation() const { return m_freezeRotation; }
    void SetFreezeRotation(bool freezeRotation);
};
