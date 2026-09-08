#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/PlayerHit.h"
#include "Script/Player.h"

void PlayerHit::OnStart()
{
    m_player = GetComponent<Player>();
    if (m_player) return;

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
}

void PlayerHit::OnHitBegin(const HitInfo& hitInfo)
{
    GameObject* otherObject = hitInfo.objectB;

    if (otherObject && otherObject->GetTag() == "Enemy")
    {
        otherObject->Destroy();
    }
}
