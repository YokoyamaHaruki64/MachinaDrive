#pragma once

#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"

class WaveDirector;

// Player接触時にWaveの残り時間を回復する、敵ドロップ用のワールドアイテム。
class TimeRecoveryItem final : public Script
{
    GM_SCRIPT(TimeRecoveryItem, "Time Recovery Item")
    USE_FIELDS

    float m_recoveryTime = 3.0f;
    float m_lifetime = 12.0f;
    PrefabReference m_idleVfxPrefab{ "Assets/Prefabs/VFX_TimeItemIdle.prefab" };
    PrefabReference m_pickupVfxPrefab{ "Assets/Prefabs/VFX_TimeRecoveryPickup.prefab" };
    float m_elapsedTime = 0.0f;
    bool m_isCollected = false;
    WaveDirector* m_waveDirector = nullptr;
    GameObjectReference m_idleVfx;

    float m_amplitude = 0.25f;
    float m_frequency = 1.0f;

    Vector3 m_initialPosition;

    void DestroyIdleVfx();

public:
    void OnStart() override;
    void Update() override;
    void OnHitBegin(const HitInfo& hitInfo) override;
    void OnDestroy() override;
};
