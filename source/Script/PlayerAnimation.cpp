#include "pch.h"

#include "Component/Transform.h"
#include "Script/Player.h"
#include "Script/PlayerAnimation.h"
#include "System/Core/GameObject.h"
#include "System/Core/GM_Time.h"
#include "System/Diagnostics/Logger.h"

FIELDS_BEGIN(PlayerAnimation)
    GM_FIELD(m_floatHeight);
    GM_FIELD(m_floatSpeed);
    GM_FIELD(m_idleYawSpeed);
    GM_FIELD(m_atkReadyRotSpeed);
    //GM_FIELD(m_attackReadyPitchAngle);
    //GM_FIELD(m_attackReadyRollAngle);
    //GM_FIELD(m_attackReadyPitchSpeed);
    //GM_FIELD(m_attackReadyRollSpeed);
    GM_FIELD(m_atkStartRotSpeed);
    GM_FIELD(m_attackSpinDecay);
    //GM_FIELD(m_attackingPitchAngle);
    //GM_FIELD(m_attackingRollAngle);
    //GM_FIELD(m_attackingPitchSpeed);
    //GM_FIELD(m_attackingRollSpeed);
    GM_FIELD(m_rotationSpeedTransitionRate);
FIELDS_END()

float PlayerAnimation::Approach(
    float current,
    float target,
    float rate,
    float deltaTime)
{
    // 指定されたrateに従ってcurrentをtargetに滑らかに近づける
    const float blend = 1.0f - std::exp(-rate * deltaTime);
    return Math::Lerp(current, target, blend);
}

void PlayerAnimation::ApplyAnimation()
{
    Transform* modelTransform = transform();
    if (!modelTransform)
    {
        return;
    }

    const Vector3 floatingOffset(
        0.0f,
        std::sin(m_elapsedTime * m_floatSpeed) * m_floatHeight,
        0.0f);
    modelTransform->SetLocalPosition(m_initialLocalPosition + floatingOffset);

    const Quaternion animationRotation = Quaternion::EulerToQuat(m_rotationEuler);
    modelTransform->SetLocalRotation(m_initialLocalRotation * animationRotation);
}

void PlayerAnimation::OnStart()
{
    Transform* modelTransform = transform();
    if (modelTransform)
    {
        m_initialLocalPosition = modelTransform->GetLocalPosition();
        m_initialLocalRotation = modelTransform->GetLocalRotation();
        m_rotationEuler = m_initialLocalRotation.QuatToEuler();
    }

    GameObject* playerObject = m_owner ? m_owner->GetParent() : nullptr;
    m_player = playerObject ? playerObject->GetComponent<Player>() : nullptr;
    if (!m_player)
    {
        Logger::Error("PlayerAnimation: 親GameObjectにPlayerコンポーネントが見つかりません。");
        return;
    }

}

void PlayerAnimation::StartAttackAnimation()
{
    m_rotationSpeed = Random::OnUnitSphere() * m_atkStartRotSpeed;
}

void PlayerAnimation::StartAttackReadyAnimation()
{
    m_rotationSpeed = Random::OnUnitSphere() * m_atkReadyRotSpeed;
}

void PlayerAnimation::Update()
{
    if (!m_player || !transform())
    {
        return;
    }

    const float deltaTime = Time::GetDeltaTime();
    m_elapsedTime += deltaTime;

    switch (m_player->GetAttackState())
    {
        case Player::Idle:
            m_rotationSpeed = Vector3(0.0f, m_idleYawSpeed, 0.0f);
            break;

        case Player::AttackReady:
            break;

        case Player::Attacking:
            m_rotationSpeed -= m_rotationSpeed * m_attackSpinDecay * deltaTime;
            break;

        case Player::AttackEnd:
            // 攻撃終了時に回転速度を減衰させる
            m_rotationSpeed.x = Approach(m_rotationSpeed.x, 0.0f, m_rotationSpeedTransitionRate, deltaTime);
            m_rotationSpeed.y = Approach(m_rotationSpeed.y, m_idleYawSpeed, m_rotationSpeedTransitionRate, deltaTime);
            m_rotationSpeed.z = Approach(m_rotationSpeed.z, 0.0f, m_rotationSpeedTransitionRate, deltaTime);
            break;

        default:
            m_rotationSpeed = Vector3(0.0f, m_idleYawSpeed, 0.0f);
            break;
    }

    m_rotationEuler += m_rotationSpeed * deltaTime;
    ApplyAnimation();
}
