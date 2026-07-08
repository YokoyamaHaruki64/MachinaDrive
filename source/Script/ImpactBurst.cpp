#include "pch.h"

#include "Script/ImpactBurst.h"

#include "System/manager.h"
#include "Script/EffectConfig.h"
#include "Component/ParticleSystem.h"

void ImpactBurst::OnStart()
{
    auto& config = Manager::GetFirstObjectByComponent<EffectConfig>()->
        GetComponent<EffectConfig>()->GetImpactBurstConfig();
    auto emitter = m_owner->GetComponent<ParticleEmitter>();
    ParticleEmitter::EmitterSettings settings;
    settings = emitter->GetSettings();

    settings.burst = true;
    settings.burstTime = 0.2f;
    settings.finishedDestroy = true;
    // directionは外部から設定される
    settings.velocityType = ParticleEmitter::VelocityType::Cone;

    settings.burstCount = config.burstCount;
    settings.lifetimeMin = config.lifetimeMin;
    settings.lifetimeMax = config.lifetimeMax;
    settings.coneAngle = config.coneAngle;
    settings.speedMin = config.speedMin;
    settings.speedMax = config.speedMax;
    settings.drag = config.drag;
    settings.gravity = Vector3(0.0f, config.gravity, 0.0f);
    settings.size = Vector3(config.size, config.size, config.size);

    settings.colorOverLifetime = config.colorCurve;
    settings.scaleOverLifetime = config.scaleCurve;


    emitter->SetSettings(settings);
}
