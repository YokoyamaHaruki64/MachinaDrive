#pragma once

#include "System/Core/Script.h"

// Playerの子Modelだけを動かし、親の物理形状を固定したまま見た目をアニメーションする。
class PlayerAnimation : public Script
{
    GM_SCRIPT(PlayerAnimation, "Player Animation");

    USE_FIELDS

private:
    // 角度・角速度はラジアン、時間は秒で指定する。
    // 要素の順番はPlayer::AttackState（Idle、AttackReady、Attacking、AttackEnd）に対応する。
    std::vector<Color> m_stateColors = {
        Color(0.21f, 0.85f, 1.0f, 1.0f),
        Color(1.0f, 0.83f, 0.31f, 1.0f),
        Color(1.0f, 0.29f, 0.14f, 1.0f),
        Color(0.60f, 0.42f, 1.0f, 1.0f),
    };
    float m_attackReadyBlinkSpeed = 5.0f;
    float m_attackReadyBlinkAmount = 0.35f;
    float m_attackingBlinkSpeed = 16.0f;
    float m_attackingBlinkAmount = 0.70f;

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
    ModelRenderer* m_modelRenderer = nullptr;
    Vector3 m_initialLocalPosition = Vector3::Zero();
    Quaternion m_initialLocalRotation = Quaternion::Identity();

    float m_elapsedTime = 0.0f;
    Quaternion m_animationRotation = Quaternion::Identity();
    Vector3 m_rotationSpeed = Vector3(0.0f, m_idleYawSpeed, 0.0f);

    static float Approach(float current, float target, float rate, float deltaTime);
    void UpdateRotation(float deltaTime);
    void ApplyCoreColor();
    void ApplyAnimation();

public:
    // PlayerがAttackingへ遷移した瞬間に呼び出し、攻撃回転を初期化する。
    void StartAttackAnimation();
    void StartAttackReadyAnimation();

    void OnStart() override;
    void Update() override;
};
