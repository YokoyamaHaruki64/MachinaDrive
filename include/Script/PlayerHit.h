#pragma once

#include "System/Core/Script.h"
#include "System/Core/GameObject.h"

class PlayerHit : public Script
{
    GM_SCRIPT(PlayerHit, "PlayerHit");

private:
    class Player* m_player = nullptr;
public:
    void OnStart() override;
    void OnHitBegin(const HitInfo& hitInfo) override;

};
