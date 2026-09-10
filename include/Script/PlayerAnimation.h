#pragma once

#include "System/Core/Script.h"

class Player;

// Playerの子Modelだけを動かし、親の物理形状を固定したまま見た目をアニメーションする。
class PlayerAnimation : public Script
{
    GM_SCRIPT(PlayerAnimation, "Player Animation");

    USE_FIELDS

private:
    // 角度・角速度は、時間は秒で指定する。
    float m_floatHeight = 0.08f;
    float m_floatSpeed = 2.0f;

    float m_idleYawSpeed = 0.65f;
    float m_atkReadyRotSpeed = 2.4f;
    float m_attackReadyPitchAngle = 0.16f;
    float m_attackReadyRollAngle = 0.22f;
    float m_attackReadyPitchSpeed = 3.2f;
    float m_attackReadyRollSpeed = 2.6f;

    float m_atkStartRotSpeed = 20.0f;
    float m_attackSpinDecay = 4.5f;
    float m_attackingPitchAngle = 0.10f;
    float m_attackingRollAngle = 0.14f;
    float m_attackingPitchSpeed = 6.0f;
    float m_attackingRollSpeed = 5.0f;

    float m_rotationSpeedTransitionRate = 8.0f;

    Player* m_player = nullptr;
    Vector3 m_initialLocalPosition = Vector3::Zero();
    Quaternion m_initialLocalRotation = Quaternion::Identity();

    float m_elapsedTime = 0.0f;
    Vector3 m_rotationEuler = Vector3::Zero();
    Vector3 m_rotationSpeed = Vector3(0.0f, m_idleYawSpeed, 0.0f);

    static float Approach(float current, float target, float rate, float deltaTime);
    void ApplyAnimation();

public:
    // PlayerがAttackingへ遷移した瞬間に呼び出し、攻撃回転を初期化する。
    void StartAttackAnimation();
    void StartAttackReadyAnimation();

    void OnStart() override;
    void Update() override;
};
