#include "pch.h"

#include "Script/ImpactBurst.h"

#include "System/manager.h"
#include "Component/ParticleSystem.h"

void ImpactBurst::OnStart()
{
    auto emitter = m_owner->GetComponent<ParticleEmitter>();
    ParticleEmitter::EmitterSettings settings;
    settings = emitter->GetSettings();

    settings.burst = true;
    settings.burstCount = 100;
    settings.burstTime = 0.2f;
    settings.finishedDestroy = true;

    settings.lifetimeMin = 0.3f;
    settings.lifetimeMax = 0.8f;

    settings.colorOverLifetime = {
        {// R
            {0.0f,0.0f},
            {0.2f,0.5f},
            {0.5f,1.0f},
        },
        {// G
            {0.0f,0.0f},
            {0.5f,0.5f},
            {1.0f,1.0f},
        },
        {// B
            {1.0f,0.0f},
            {1.0f,0.5f},
            {0.5f,1.0f},
        },
        {// A
            { 1.0f, 1.0f },
            { 0.5f, 0.5f },
            { 0.2f, 0.0f }
        }
   };

    settings.scaleOverLifetime =
    {
        { 1.0f, 0.0f },
        { 0.8f, 0.5f },
        { 0.0f, 1.0f }
    };

    // directionは外部から設定される
    settings.velocityType = ParticleEmitter::VelocityType::Cone;
    settings.coneAngle = 24.0f;
    settings.speedMin = 3.0f;
    settings.speedMax = 8.0f;
    settings.drag = 0.0f;
    settings.gravity = Vector3(0.0f, 0.0f, 0.0f);
    settings.size = Vector3(0.4f, 0.4f, 0.4f);

    emitter->SetSettings(settings);
}
