#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/CollisionVfx.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(CollisionVfx)
    GM_FIELD(m_coneAngle);
    GM_FIELD(m_burstTime);
    GM_FIELD(m_burstCount);
    GM_FIELD(m_maxParticles);
    GM_FIELD(m_particleLifetimeMin);
    GM_FIELD(m_particleLifetimeMax);
    GM_FIELD(m_speedMin);
    GM_FIELD(m_speedMax);
    GM_FIELD(m_gravity);
    GM_FIELD(m_drag);
    GM_FIELD(m_particleSize);
    GM_FIELD(m_orangeEmissionColor);
    GM_FIELD(m_yellowOrangeEmissionColor);
    GM_FIELD(m_redOrangeEmissionColor);
    GM_FIELD_TYPE(m_scaleOverLifetime, InspectorFieldType::Curve);
    GM_FIELD_TYPE(m_orangeColorOverLifetime, InspectorFieldType::Curve);
    GM_FIELD_TYPE(m_yellowOrangeColorOverLifetime, InspectorFieldType::Curve);
    GM_FIELD_TYPE(m_redOrangeColorOverLifetime, InspectorFieldType::Curve);
FIELDS_END()

void CollisionVfx::OnStart()
{
    m_coneAngle = Math::Max(m_coneAngle, 0.0f);
    m_burstTime = Math::Max(m_burstTime, 0.001f);
    m_burstCount = Math::Max(m_burstCount, 1);
    m_maxParticles = Math::Max(m_maxParticles, m_burstCount);
    m_particleLifetimeMin = Math::Max(m_particleLifetimeMin, 0.01f);
    m_particleLifetimeMax = Math::Max(m_particleLifetimeMax, m_particleLifetimeMin);
    m_speedMin = Math::Max(m_speedMin, 0.0f);
    m_speedMax = Math::Max(m_speedMax, m_speedMin);
    m_drag = Math::Max(m_drag, 0.0f);
    for (TransformBase* childTransform : transform()->GetChildren())
    {
        if (!childTransform || !childTransform->GetOwner())
        {
            continue;
        }

        GameObject* child = childTransform->GetOwner();
        ParticleEmitter* emitter = child->GetComponent<ParticleEmitter>();
        if (!emitter)
        {
            continue;
        }

        const std::string& childName = child->GetName();
        uint8_t colorVariant = 0;
        if (childName.find("Yellow") != std::string::npos
            || childName.find("Gold") != std::string::npos)
        {
            colorVariant = 1;
        }
        else if (childName.find("Red") != std::string::npos)
        {
            colorVariant = 2;
        }

        const EmitterSlot slot{ emitter, colorVariant };
        if (childName.find("Player") != std::string::npos)
        {
            m_playerEmitters.push_back(slot);
        }
        else if (childName.find("Enemy") != std::string::npos)
        {
            m_enemyEmitters.push_back(slot);
        }
    }

    if (m_playerEmitters.empty() || m_enemyEmitters.empty())
    {
        Logger::Error("CollisionVfx: Player／Enemy側のParticleEmitterが見つかりません。");
    }
    else
    {
        Logger::Log("CollisionVfx: Player／Enemy側のParticleEmitterを検出しました。");
    }
}

const CurveColor& CollisionVfx::GetColorCurve(uint8_t colorVariant) const
{
    switch (colorVariant)
    {
    case 1:
        return m_yellowOrangeColorOverLifetime;
    case 2:
        return m_redOrangeColorOverLifetime;
    default:
        return m_orangeColorOverLifetime;
    }
}

const Color& CollisionVfx::GetEmissionColor(uint8_t colorVariant) const
{
    switch (colorVariant)
    {
    case 1:
        return m_yellowOrangeEmissionColor;
    case 2:
        return m_redOrangeEmissionColor;
    default:
        return m_orangeEmissionColor;
    }
}

void CollisionVfx::ApplyEmitterSettings(
    ParticleEmitter* emitter,
    const Vector3& direction,
    const CurveColor& colorOverLifetime,
    const Color& emissionColor)
{
    if (!emitter)
    {
        return;
    }

    ParticleEmitter::EmitterSettings settings = emitter->GetSettings();
    settings.maxParticles = m_maxParticles;
    settings.burst = true;
    settings.burstCount = m_burstCount;
    settings.burstTime = m_burstTime;
    // 親のVfxLifetimeが全Emitterの完了を監視するため、Emitter自身は破棄しない。
    settings.finishedDestroy = false;
    settings.lifetimeMin = m_particleLifetimeMin;
    settings.lifetimeMax = m_particleLifetimeMax;
    settings.gravity = m_gravity;
    settings.drag = m_drag;
    settings.offset = direction * 0.08f;
    settings.size = m_particleSize;
    settings.emissionColor = emissionColor;
    settings.scaleOverLifetime = m_scaleOverLifetime;
    settings.colorOverLifetime = colorOverLifetime;
    settings.randomizeSpeed = true;
    settings.speedMin = m_speedMin;
    settings.speedMax = m_speedMax;
    settings.velocityType = ParticleEmitter::Cone;
    settings.direction = direction;
    settings.coneAngle = m_coneAngle;
    settings.spawnType = ParticleEmitter::Point;
    emitter->SetSettings(settings);
}

void CollisionVfx::Configure(const Vector3& normal, const Vector3& contactPosition)
{
    if (transform())
    {
        transform()->SetPosition(contactPosition);
    }

    Vector3 playerDirection = normal.Normalized();
    if (playerDirection.LengthSqr() <= 0.0001f)
    {
        playerDirection = Vector3::Up();
    }
    const Vector3 enemyDirection = playerDirection * -1.0f;

    for (const EmitterSlot& slot : m_playerEmitters)
    {
        ApplyEmitterSettings(
            slot.emitter,
            playerDirection,
            GetColorCurve(slot.colorVariant),
            GetEmissionColor(slot.colorVariant));
    }
    for (const EmitterSlot& slot : m_enemyEmitters)
    {
        ApplyEmitterSettings(
            slot.emitter,
            enemyDirection,
            GetColorCurve(slot.colorVariant),
            GetEmissionColor(slot.colorVariant));
    }

}
