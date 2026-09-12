#pragma once

#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"

class AttackReadyVfx final : public Script
{
    GM_SCRIPT(AttackReadyVfx, "Attack Ready VFX")
    USE_FIELDS

    PrefabReference m_headPrefab{ "Assets/Prefabs/VFX_AttackChargeHead.prefab" };
    GameObjectReference m_player;

    float m_spawnInterval = 0.16f;
    int m_maxActiveHeads = 8;
    float m_spawnRadiusMin = 3.0f;
    float m_spawnRadiusMax = 4.5f;
    float m_spawnHeightMin = 0.4f;
    float m_spawnHeightMax = 2.2f;
    float m_spawnTimer = 0.0f;

    void SpawnHead();

public:
    void OnStart() override;
    void Update() override;
};
