#include "pch.h"

#include "Component/Rigidbody.h"

#include "System/Editor/FieldRegistry.h"
#include "System/Core/Physics.h"

Rigidbody::~Rigidbody()
{
    // OnDestroyが呼ばれない経路でも、Physicsの登録集合にポインタを残さない。
    Physics::UnregisterRigidbody(this);
}

void Rigidbody::OnLoaded()
{
    Physics::RegisterRigidbody(this);
}

void Rigidbody::OnDestroy()
{
    Physics::UnregisterRigidbody(this);
}

void Rigidbody::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_motionType);
    GM_FIELD_ACCESSOR(float, "m_mass", GetMass(), SetMass(value), All);
    GM_FIELD(m_velocity);
    GM_FIELD_ACCESSOR(float, "m_bounciness", GetBounciness(), SetBounciness(value), All);
    GM_FIELD_ACCESSOR(float, "m_friction", GetFriction(), SetFriction(value), All);
    GM_FIELD(m_useGravity);
    GM_FIELD(m_allowSleep);
    GM_FIELD(m_freezeRotation);
}

void Rigidbody::SetMotionType(MotionType motionType)
{
    m_motionType = static_cast<int>(motionType);
    Physics::MarkDirty();
}

void Rigidbody::SetMass(float mass)
{
    m_mass = std::max(mass, 0.0f);
    Physics::MarkDirty();
}

void Rigidbody::SetBounciness(float bounciness)
{
    m_bounciness = Math::Clamp(bounciness, 0.0f, 1.0f);
    Physics::MarkDirty();
}

void Rigidbody::SetFriction(float friction)
{
    m_friction = std::max(friction, 0.0f);
    Physics::MarkDirty();
}

void Rigidbody::SetUseGravity(bool useGravity)
{
    m_useGravity = useGravity;
    Physics::MarkDirty();
}

void Rigidbody::SetAllowSleep(bool allowSleep)
{
    m_allowSleep = allowSleep;
    Physics::MarkDirty();
}

void Rigidbody::SetFreezeRotation(bool freezeRotation)
{
    m_freezeRotation = freezeRotation;
    Physics::MarkDirty();
}
