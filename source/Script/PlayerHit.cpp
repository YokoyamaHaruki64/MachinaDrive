#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/PlayerHit.h"
#include "Script/Health.h"
#include "Script/Player.h"
#include "Script/CameraController.h"
#include "Script/ComboUI.h"

FIELDS_BEGIN(PlayerHit)
    GM_FIELD(m_comboUIObject);
    GM_FIELD(m_shakeDuration);
    GM_FIELD(m_shakeMagnitude);
    GM_FIELD(m_hitStopDuration);
FIELDS_END()

void PlayerHit::OnStart()
{
    m_player = GetComponent<Player>();
    m_cameraController = Manager::GetFirstObjectByComponent<CameraController>()->GetComponent<CameraController>();

    if (m_player && m_cameraController) return;

    TransformBase* parent = transform()->GetParent();

    // 親方向に辿ってPlayerコンポーネントを探す
    while (!m_player && parent)
    {

        if (parent)
        {
            m_player = parent->GetComponent<Player>();
        }
        else
        {
            Logger::Error("PlayerHit: Playerコンポーネントを持つ親GameObjectが見つかりません。");
            break;
        }

        parent = parent->GetParent();
    }

    m_comboUI = m_comboUIObject.Get() ? m_comboUIObject->GetComponent<ComboUI>() : nullptr;

    if(m_comboUI)
        m_comboUI->ResetCombo();
    else
        Logger::Error("PlayerHit: ComboUIコンポーネントを持つGameObjectが見つかりません。");
}

void PlayerHit::OnHitBegin(const HitInfo& hitInfo)
{
    GameObject* otherObject = hitInfo.objectB;

    if (otherObject && otherObject->GetTag() == "Enemy")
    {
        if(m_player && m_player->GetAttackState() != Player::Attacking)
            return;


        if (Health* health = otherObject->GetComponent<Health>())
            health->TakeDamage(1.0f);

        if(m_cameraController)  m_cameraController->Shake(m_shakeDuration, m_shakeMagnitude);

        if(m_player)            m_player->HitStop(m_hitStopDuration);

        if(m_comboUI)           m_comboUI->AddCombo();
    }
}
