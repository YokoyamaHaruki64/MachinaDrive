#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/AttackChargeHead.h"
#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(AttackChargeHead)
    GM_FIELD(m_initialSpeed);
    GM_FIELD(m_acceleration);
    GM_FIELD(m_arrivalDistance);
    GM_FIELD(m_targetOffset);
FIELDS_END()

void AttackChargeHead::OnStart()
{
    m_player = Manager::GetObjectFirstByTag("Player");
    m_initialSpeed = Math::Max(m_initialSpeed, 0.0f);
    m_acceleration = Math::Max(m_acceleration, 0.0f);
    m_arrivalDistance = Math::Max(m_arrivalDistance, 0.01f);
    m_speed = m_initialSpeed;
}

void AttackChargeHead::Update()
{
    if (!m_player || !transform())
    {
        m_owner->Destroy();
        return;
    }

    const Player* player = m_player->GetComponent<Player>();
    if (!player || player->GetAttackState() != Player::AttackReady)
    {
        m_owner->Destroy();
        return;
    }

    const Vector3 targetPosition = m_player->GetTransform()->GetPosition() + m_targetOffset;
    const Vector3 toTarget = targetPosition - transform()->GetPosition();
    const float distance = toTarget.Length();
    if (distance <= m_arrivalDistance)
    {
        m_owner->Destroy();
        return;
    }

    m_speed += m_acceleration * Time::GetDeltaTime();
    const Vector3 direction = toTarget / distance;
    transform()->Translate(direction * (m_speed * Time::GetDeltaTime()));
}
