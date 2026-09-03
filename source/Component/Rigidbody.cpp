#include "pch.h"

#include "Component/Rigidbody.h"

#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"

Rigidbody::~Rigidbody()
{
    // OnDestroyが呼ばれない経路でも、Physicsの登録集合にポインタを残さない。
    Physics::UnregisterRigidbody(this);
}

void Rigidbody::OnEnable()
{
    Physics::RegisterRigidbody(this);
}

void Rigidbody::OnDisable()
{
    Physics::UnregisterRigidbody(this);
}

void Rigidbody::OnDestroy()
{
    Physics::UnregisterRigidbody(this);
}

void Rigidbody::OnTransformChanged(const TransformChangedEvent& event)
{
    if (HasTransformChange(event.changes, TransformChange::Hierarchy))
    {
        Physics::RefreshRigidbodyState(this);
    }

    if (HasTransformChange(event.changes, TransformChange::Scale)
        || HasTransformChange(event.changes, TransformChange::Hierarchy))
    {
        // World ScaleはCollider寸法へ焼き込むため、Body配下のShapeを更新する。
        Physics::MarkBodyShapeDirty(this);
    }

    if (!HasTransformChange(event.changes, TransformChange::Scale))
    {
        Physics::MarkBodyTransformDirty(this);
    }
}

void Rigidbody::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD_ACCESSOR(int, "m_motionType", GetMotionTypeValue(), SetMotionTypeValue(value), All);
    GM_FIELD_ACCESSOR(float, "m_mass", GetMass(), SetMass(value), All);
    GM_FIELD_ACCESSOR(Vector3, "m_velocity", GetVelocity(), SetVelocity(value), All);
    GM_FIELD_ACCESSOR(float, "m_bounciness", GetBounciness(), SetBounciness(value), All);
    GM_FIELD_ACCESSOR(float, "m_friction", GetFriction(), SetFriction(value), All);
    GM_FIELD_ACCESSOR(bool, "m_useGravity", IsUseGravity(), SetUseGravity(value), All);
    GM_FIELD_ACCESSOR(bool, "m_allowSleep", IsAllowSleep(), SetAllowSleep(value), All);
    GM_FIELD_ACCESSOR(bool, "m_freezeRotation", IsFreezeRotation(), SetFreezeRotation(value), All);
}

void Rigidbody::SetMotionType(MotionType motionType)
{
    m_motionType = static_cast<int>(motionType);
    Physics::MarkBodySettingsDirty(this, true);
}

void Rigidbody::SetMotionTypeValue(int motionType)
{
    m_motionType = motionType;
    Physics::MarkBodySettingsDirty(this, true);
}

void Rigidbody::SetVelocity(const Vector3& velocity)
{
    m_velocity = velocity;
    Physics::MarkBodyVelocityDirty(this);
}

void Rigidbody::AddVelocity(const Vector3& velocity)
{
    m_velocity += velocity;
    Physics::MarkBodyVelocityDirty(this);
}

void Rigidbody::SetMass(float mass)
{
    m_mass = std::max(mass, 0.0f);
    Physics::MarkBodySettingsDirty(this, true);
}

void Rigidbody::SetBounciness(float bounciness)
{
    m_bounciness = Math::Clamp(bounciness, 0.0f, 1.0f);
    Physics::MarkBodySettingsDirty(this, false);
}

void Rigidbody::SetFriction(float friction)
{
    m_friction = std::max(friction, 0.0f);
    Physics::MarkBodySettingsDirty(this, false);
}

void Rigidbody::SetUseGravity(bool useGravity)
{
    m_useGravity = useGravity;
    Physics::MarkBodySettingsDirty(this, false);
}

void Rigidbody::SetAllowSleep(bool allowSleep)
{
    m_allowSleep = allowSleep;
    Physics::MarkBodySettingsDirty(this, true);
}

void Rigidbody::SetFreezeRotation(bool freezeRotation)
{
    m_freezeRotation = freezeRotation;
    Physics::MarkBodySettingsDirty(this, true);
}
