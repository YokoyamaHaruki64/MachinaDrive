#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/PlayerHit.h"
#include "Script/CollisionVfx.h"
#include "Script/Enemy.h"
#include "Script/Player.h"
#include "Script/CameraController.h"
#include "Script/UI/GameHud.h"

FIELDS_BEGIN(PlayerHit)
    GM_FIELD(m_collisionVfxPrefab);
    GM_FIELD(m_shakeDuration);
    GM_FIELD(m_shakeMagnitude);
    GM_FIELD(m_hitStopDuration);
FIELDS_END()

void PlayerHit::OnStart()
{
    m_hitAudios[0] = m_owner->AddComponent<Audio>("Assets/Local/Sounds/SE_Hit01.mp3");
    m_hitAudios[1] = m_owner->AddComponent<Audio>("Assets/Local/Sounds/SE_Hit02.mp3");

    for(int i = 0; i < m_hitAudios.size(); ++i)
    {
        if (m_hitAudios[i])
        {
            m_hitAudios[i]->SetGain(5.0f);
        }
    }

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

        Logger::Log("PlayerHit: Enemyへの有効ヒットを検出しました。");

        for (Audio* hitAudio : m_hitAudios)
        {
            if (hitAudio)
            {
                hitAudio->Play();
            }
        }

        if (m_collisionVfxPrefab)
        {
            GameObject* vfxObject = Manager::Instantiate(
                m_collisionVfxPrefab.GetPath(),
                hitInfo.point,
                Vector3::Zero(),
                Vector3::One());
            if (vfxObject)
            {
                if (CollisionVfx* collisionVfx = vfxObject->GetComponent<CollisionVfx>())
                {
                    collisionVfx->Configure(hitInfo.normal, hitInfo.point);
                    Logger::Log("PlayerHit: 衝突VFXを設定しました。");
                }
                else
                {
                    Logger::Error("PlayerHit: VFX_CollisionにCollisionVfxがありません。");
                }
            }
            else
            {
                Logger::Error("PlayerHit: VFX_Collisionの生成に失敗しました。");
            }
        }

        if(m_cameraController)  m_cameraController->Shake(m_shakeDuration, m_shakeMagnitude);

        if(m_player)            m_player->HitStop(m_hitStopDuration);

        if (m_gameHud)
        {
            m_gameHud->AddCombo();
        }
    }
}
