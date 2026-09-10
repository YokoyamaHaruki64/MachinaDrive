#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/MovingEnemy.h"

#include <algorithm>

void MovingEnemy::RegisterFields(FieldRegistry& fields)
{
    Enemy::RegisterFields(fields);
    GM_FIELD(m_moveDuration);
    GM_FIELD(m_stopDuration);
    GM_FIELD(m_moveSpeed);
}

void MovingEnemy::OnStart()
{
    Enemy::OnStart();
    m_rigidbody = GetComponent<Rigidbody>();
    if (!m_rigidbody)
    {
        Logger::Error("MovingEnemy: Rigidbody component not found.");
        return;
    }

    if (m_playerObject && m_playerObject->GetTransform())
    {
        // 開始直後からプレイヤーへ向かう移動フェーズに入る。
        BeginMoving(m_playerObject->GetTransform()->GetPosition());
    }
}

void MovingEnemy::UpdateBehavior(const Vector3& playerPosition)
{
    if (!m_rigidbody)
    {
        return;
    }

    if (m_isMoving)
    {
        // プレイヤーが動いても、移動フェーズ中は常に現在位置へ向かう。
        SetVelocityTowardsPlayer(playerPosition);
    }

    m_stateTimer -= Time::GetDeltaTime();
    if (m_stateTimer > 0.0f)
    {
        return;
    }

    if (m_isMoving)
    {
        BeginStopping();
    }
    else
    {
        BeginMoving(playerPosition);
    }
}

void MovingEnemy::SetVelocityTowardsPlayer(const Vector3& playerPosition)
{
    Vector3 direction = playerPosition - transform()->GetPosition();
    direction.y = 0.0f;
    if (direction.LengthSqr() <= STOP_DISTANCE_FROM_PLAYER * STOP_DISTANCE_FROM_PLAYER)
    {
        // 接触して押し合い続けないよう、所定距離内では物理速度を明示的に止める。
        m_rigidbody->SetVelocity(Vector3::Zero());
        return;
    }

    direction.Normalize();

    // Dynamic Rigidbodyの速度だけを変更し、Transformを直接操作しない。
    m_rigidbody->SetVelocity(direction * std::max(m_moveSpeed, 0.0f));
}

void MovingEnemy::BeginMoving(const Vector3& playerPosition)
{
    m_isMoving = true;
    SetVelocityTowardsPlayer(playerPosition);
    m_stateTimer = Math::Max(m_moveDuration, 0.01f);
}

void MovingEnemy::BeginStopping()
{
    m_isMoving = false;
    m_rigidbody->SetVelocity(Vector3::Zero());
    m_stateTimer = Math::Max(m_stopDuration, 0.01f);
}
