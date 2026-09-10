#pragma once

#include "System/Core/Script.h"

// Enemyの子Modelだけを動かし、親の物理形状を固定したまま浮遊感を演出する。
class EnemyAnimation final : public Script
{
    GM_SCRIPT(EnemyAnimation, "Enemy Animation")

    USE_FIELDS

private:
    float m_floatHeight = 0.08f;
    float m_floatSpeed = 2.0f;

    Vector3 m_initialLocalPosition = Vector3::Zero();
    float m_elapsedTime = 0.0f;

public:
    void OnStart() override;
    void Update() override;
};
