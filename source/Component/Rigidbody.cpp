#include "pch.h"

#include "Component/Rigidbody.h"

#include "System/Editor/FieldRegistry.h"

void Rigidbody::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD_ACCESSOR(float, "m_mass", GetMass(), SetMass(value), All);
    GM_FIELD(m_velocity);
    GM_FIELD_ACCESSOR(float, "m_bounciness", GetBounciness(), SetBounciness(value), All);
}

void Rigidbody::SetMass(float mass)
{
    m_mass = std::max(mass, 0.0f);
}

void Rigidbody::SetBounciness(float bounciness)
{
    m_bounciness = Math::Clamp(bounciness, 0.0f, 1.0f);
}
