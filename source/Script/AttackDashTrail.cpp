#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/AttackDashTrail.h"
#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(AttackDashTrail)
    GM_FIELD(m_offset);
    GM_FIELD(m_lingerDuration);
FIELDS_END()

void AttackDashTrail::OnStart()
{
    m_player = Manager::GetObjectFirstByTag("Player");
    m_lingerDuration = Math::Max(m_lingerDuration, 0.0f);
    m_lingerTimer = 0.0f;

    if (m_player && m_player->GetTransform())
    {
        transform()->SetPosition(m_player->GetTransform()->GetPosition() + m_offset);
    }
}

void AttackDashTrail::Update()
{
    if (!m_player || !transform())
    {
        m_owner->Destroy();
        return;
    }

    const Player* player = m_player->GetComponent<Player>();
    if (player && player->GetAttackState() == Player::Attacking)
    {
        m_lingerTimer = 0.0f;
        return;
    }

    // Trailの頂点寿命と同程度だけ残し、急停止後にも速度の余韻を読ませる。
    m_lingerTimer += Time::GetDeltaTime();
    if (m_lingerTimer >= m_lingerDuration)
    {
        m_owner->Destroy();
    }
}

void AttackDashTrail::LateUpdate()
{
    if (!m_player || !transform())
    {
        return;
    }

    const Player* player = m_player->GetComponent<Player>();
    if (player && player->GetAttackState() == Player::Attacking)
    {
        // Physics同期後のPlayer座標を、Draw時に記録するTrailへ渡す。
        transform()->SetPosition(m_player->GetTransform()->GetPosition() + m_offset);
    }
}
