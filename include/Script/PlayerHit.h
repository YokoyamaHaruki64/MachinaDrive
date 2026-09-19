#pragma once

#include <array>

#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"
#include "System/Core/GameObject.h"

class PlayerHit : public Script
{
    GM_SCRIPT(PlayerHit, "PlayerHit");
    USE_FIELDS
private:
    std::array<class Audio*, 2> m_hitAudios{};
    class Player* m_player = nullptr;
    class CameraController* m_cameraController = nullptr;

    class GameHud* m_gameHud = nullptr;
    PrefabReference m_collisionVfxPrefab{ "Assets/Prefabs/VFX_Collision.prefab" };

    float m_shakeDuration = 0.5f;
    float m_shakeMagnitude = 0.5f;
    float m_hitStopDuration = 0.3f;

public:
    void OnStart() override;
    void OnHitBegin(const HitInfo& hitInfo) override;

};
