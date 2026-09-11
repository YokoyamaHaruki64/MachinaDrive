#pragma once

#include "System/Core/Script.h"
#include "System/Core/GameObject.h"

class PlayerHit : public Script
{
    GM_SCRIPT(PlayerHit, "PlayerHit");
    USE_FIELDS
private:
    class Player* m_player = nullptr;
    class CameraController* m_cameraController = nullptr;

    class GameHud* m_gameHud = nullptr;

    float m_shakeDuration = 0.5f;
    float m_shakeMagnitude = 0.5f;
    float m_hitStopDuration = 0.3f;

public:
    void OnStart() override;
    void OnHitBegin(const HitInfo& hitInfo) override;

};
