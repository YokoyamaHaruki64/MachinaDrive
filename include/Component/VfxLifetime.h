#pragma once

#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"

#include <unordered_set>

class ParticleEmitter;

class VfxLifetime final : public Component
{
    REGISTER_COMPONENT(VfxLifetime, "VFX Lifetime");

    std::unordered_set<ParticleEmitter*> m_pendingEmitters;

public:
    explicit VfxLifetime(GameObject* owner)
        : Component(owner)
    {
    }

    void OnDestroy() override;

    void RegisterEmitter(ParticleEmitter& emitter);
    void NotifyEmitterFinished(ParticleEmitter& emitter);
};
