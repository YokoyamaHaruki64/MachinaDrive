#include "pch.h"

#include "Component/ComponentInc.h"

#include "Common/Random.h"
#include "Script/MovingEnemy.h"

#include <cmath>

void MovingEnemy::RegisterFields(FieldRegistry& fields)
{
    Enemy::RegisterFields(fields);
    GM_FIELD(m_amplitudeMin);
    GM_FIELD(m_amplitudeMax);
    GM_FIELD(m_frequencyMin);
    GM_FIELD(m_frequencyMax);
}

void MovingEnemy::OnStart()
{
    Enemy::OnStart();
    m_rigidbody = GetComponent<Rigidbody>();
    if (!m_rigidbody)
    {
        Logger::Error("MovingEnemy: Rigidbody component not found.");
        return;
    }

    SelectMovementParameters();
}

void MovingEnemy::SelectMovementParameters()
{
    const float angle = Random::Range(0.0f, Math::PI * 2.0f);
    m_moveDirection = Vector3(std::cos(angle), 0.0f, std::sin(angle));
    m_amplitude = Random::Range(
        Math::Min(m_amplitudeMin, m_amplitudeMax),
        Math::Max(m_amplitudeMin, m_amplitudeMax));
    m_frequency = Random::Range(
        Math::Min(m_frequencyMin, m_frequencyMax),
        Math::Max(m_frequencyMin, m_frequencyMax));
    m_elapsedTime = 0.0f;
}

void MovingEnemy::UpdateBehavior(const Vector3&)
{
    if (!m_rigidbody)
    {
        return;
    }

    m_elapsedTime += Time::GetDeltaTime();
    const float angularFrequency = Math::PI * 2.0f * Math::Max(0.0f, m_frequency);
    const float speed = m_amplitude * angularFrequency * std::cos(angularFrequency * m_elapsedTime);
    m_rigidbody->SetVelocity(m_moveDirection * speed);
}
