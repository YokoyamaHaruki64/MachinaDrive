#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/PlayerHit.h"
#include "Script/Enemy.h"
#include "Script/Player.h"
#include "Script/CameraController.h"
#include "Script/UI/GameHud.h"

FIELDS_BEGIN(PlayerHit)
    GM_FIELD(m_shakeDuration);
    GM_FIELD(m_shakeMagnitude);
    GM_FIELD(m_hitStopDuration);
FIELDS_END()

void PlayerHit::OnStart()
{
    m_player = GetComponent<Player>();
    m_cameraController = Manager::GetFirstObjectByComponent<CameraController>()->GetComponent<CameraController>();

    if (!m_player)
    {
        TransformBase* parent = transform()->GetParent();

        // 親方向に辿ってPlayerコンポーネントを探す
        while (!m_player && parent)
        {
            m_player = parent->GetComponent<Player>();
            parent = parent->GetParent();
        }
    }

    if (GameObject* hudObject = Manager::GetFirstObjectByComponent<GameHud>())
    {
        m_gameHud = hudObject->GetComponent<GameHud>();
    }

    if (m_gameHud)
    {
        m_gameHud->ResetCombo();
    }
    else
        Logger::Error("PlayerHit: GameHudコンポーネントが見つかりません。");
}

void PlayerHit::OnHitBegin(const HitInfo& hitInfo)
{
    GameObject* otherObject = hitInfo.objectB;

    if (otherObject && otherObject->GetTag() == "Enemy")
    {
        if(m_player && m_player->GetAttackState() != Player::Attacking)
            return;


        Enemy* enemy = otherObject->GetComponent<Enemy>();
        if (!enemy || !enemy->TakeDamage(1.0f))
            return;

        if(m_cameraController)  m_cameraController->Shake(m_shakeDuration, m_shakeMagnitude);

        if(m_player)            m_player->HitStop(m_hitStopDuration);

        if (m_gameHud)
        {
            m_gameHud->AddCombo();
        }
    }
}
