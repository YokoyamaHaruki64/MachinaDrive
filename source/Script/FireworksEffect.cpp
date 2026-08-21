#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/FireworksEffect.h"

#include "Script/EffectConfig.h"

void FireworksEffect::OnStart()
{
    auto emitter = m_owner->GetComponent<ParticleEmitter>();
    auto& config = Manager::GetFirstObjectByComponent<EffectConfig>()
        ->GetComponent<EffectConfig>()->GetFireWorkConfig();
    ParticleEmitter::EmitterSettings settings;
    settings = emitter->GetSettings();

    settings.burst = true;
    settings.finishedDestroy = true;
    settings.burstTime = 0.0f;
    settings.velocityType = ParticleEmitter::VelocityType::SphereVel;

    settings.burstCount = config.burstCount;
    settings.lifetimeMin = config.lifetimeMin;
    settings.lifetimeMax = config.lifetimeMax;
    settings.speedMin = config.speedMin;
    settings.speedMax = config.speedMax;
    settings.drag = config.drag;
    settings.gravity = Vector3(0.0f, config.gravity, 0.0f);
    settings.size = Vector3(config.size, config.size, config.size);

    settings.colorOverLifetime = config.colorCurve;
    settings.scaleOverLifetime = config.scaleCurve;

    emitter->SetSettings(settings);
}

void FireworksEffect::Update()
{
}
