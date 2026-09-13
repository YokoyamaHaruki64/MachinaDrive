#include "pch.h"

#include "Component/ComponentInc.h"

#include "Component/VfxLifetime.h"
#include "Component/ParticleSystem.h"

void VfxLifetime::OnDestroy()
{
    for (ParticleEmitter* emitter : m_pendingEmitters)
    {
        if (emitter)
        {
            emitter->ClearVfxLifetime(this);
        }
    }

    m_pendingEmitters.clear();
}

void VfxLifetime::RegisterEmitter(ParticleEmitter& emitter)
{
    m_pendingEmitters.insert(&emitter);
}

void VfxLifetime::NotifyEmitterFinished(ParticleEmitter& emitter)
{
    m_pendingEmitters.erase(&emitter);
    if (m_pendingEmitters.empty())
    {
        // すべてのEmitterが粒子を出し切った時だけ、親Prefabをまとめて破棄する。
        m_owner->Destroy();
    }
}
