#include "pch.h"

#include "Script/FireworksEffect.h"

#include "System/manager.h"
#include "Component/ParticleSystem.h"

void FireworksEffect::OnStart()
{
    auto emitter = m_owner->GetComponent<ParticleEmitter>();
    ParticleEmitter::EmitterSettings settings;
    settings = emitter->GetSettings();

    settings.burst = true;
    settings.burstCount = 1000;
    settings.burstTime = 0.0f;
    settings.finishedDestroy = true;
    settings.lifetimeMin = 0.5f;
    settings.lifetimeMax = 1.5f;
    settings.colorOverLifetime = {
    {// R
        {1.0f,0.0f},
        {0.6f,0.5f},
        {0.5f,1.0f},
    },
    {// G
        {0.2f,0.0f},
        {0.5f,0.5f},
        {1.0f,1.0f},
    },
    {// B
        {1.0f,0.0f},
        {0.5f,0.5f},
        {0.0f,1.0f},
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

    settings.velocityType = ParticleEmitter::VelocityType::SphereVel;
    settings.speedMin = 3.0f;
    settings.speedMax = 6.0f;
    settings.drag = 3.0f;
    settings.gravity = Vector3(0.0f, -3.0f, 0.0f);
    settings.size = Vector3(0.4f, 0.4f, 0.4f);

    emitter->SetSettings(settings);
}

void FireworksEffect::Update()
{
}
