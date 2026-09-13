#pragma once

#include "System/Core/Script.h"

// 攻撃ダッシュ中だけPlayerを追従し、停止後は既存Trailの余韻を残して消える。
class AttackDashTrail final : public Script
{
    GM_SCRIPT(AttackDashTrail, "Attack Dash Trail")
    USE_FIELDS

    GameObjectReference m_player;
    Vector3 m_offset{ 0.0f, 0.35f, 0.0f };
    float m_lingerDuration = 0.25f;
    float m_lingerTimer = 0.0f;

public:
    void OnStart() override;
    void Update() override;
};
