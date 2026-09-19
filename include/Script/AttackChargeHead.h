#pragma once

#include "System/Core/Script.h"

class AttackChargeHead final : public Script
{
    GM_SCRIPT(AttackChargeHead, "Attack Charge Head")
    USE_FIELDS

    GameObjectReference m_player;

    float m_initialSpeed = 0.8f;
    float m_acceleration = 8.0f;
    float m_arrivalDistance = 0.18f;
    Vector3 m_targetOffset{ 0.0f, 0.9f, 0.0f };

    float m_speed = 0.0f;

public:
    void OnStart() override;
    void Update() override;
};
