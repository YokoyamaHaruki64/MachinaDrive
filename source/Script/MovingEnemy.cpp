#include "pch.h"

#include "Component/ComponentInc.h"

#include "Common/Random.h"
#include "Script/MovingEnemy.h"

#include <cmath>

void MovingEnemy::RegisterFields(FieldRegistry& fields)
{
    Enemy::RegisterFields(fields);
    GM_FIELD(m_teleportInterval);
    GM_FIELD(m_teleportDistance);
}

void MovingEnemy::OnStart()
{
    Enemy::OnStart();
    m_teleportTimer = m_teleportInterval;
}

void MovingEnemy::UpdateBehavior(const Vector3& playerPosition)
{
    m_teleportTimer -= Time::GetDeltaTime();
    if (m_teleportTimer > 0.0f)
    {
        return;
    }

    const float angle = Random::Range(0.0f, Math::PI_2);
    Vector3 position = playerPosition + Vector3(std::cos(angle), 0.0f, std::sin(angle)) * m_teleportDistance;
    position.y = transform()->GetPosition().y;

    Vector3 fromOrigin = position;
    fromOrigin.y = 0.0f;
    if (fromOrigin.LengthSqr() > MAX_TELEPORT_DISTANCE_FROM_ORIGIN * MAX_TELEPORT_DISTANCE_FROM_ORIGIN)
    {
        fromOrigin.Normalize();
        position.x = fromOrigin.x * MAX_TELEPORT_DISTANCE_FROM_ORIGIN;
        position.z = fromOrigin.z * MAX_TELEPORT_DISTANCE_FROM_ORIGIN;
    }

    transform()->SetPosition(position);
    m_teleportTimer = Math::Max(m_teleportInterval, 0.01f);
}
