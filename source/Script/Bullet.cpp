#include "pch.h"

#include "System/GM_Time.h"
#include "System/manager.h"
#include "Component/Transform.h"

#include "Script/Bullet.h"
#include "Script/Enemy.h"

void Bullet::Update()
{
    transform()->SetPosition(transform()->GetPosition() + m_velocity * Time::GetDeltaTime());

    // 当たり判定
    auto enemies = Manager::GetObjectsByComponent<Enemy>();
    for( auto& enemyObj : enemies)
    {
        Transform* enemyTransform = enemyObj->GetTransform();
        // 簡易的な当たり判定（距離が一定以下ならヒットとする
        float distance =
            (transform()->GetPosition() - enemyTransform->GetPosition()).LengthSqr();
        if (distance < HitRadius * HitRadius) // ヒットの閾値
        {
            enemyObj->Destroy();    // 敵を破壊
            m_owner->Destroy();     // 弾を破壊
            Manager::Instantiate(
                "ExplosionEffect",
                enemyTransform->GetPosition() + Vector3(0.0f, 0.5f, 0.0f), // 敵の位置に少し上げてエフェクトを生成
                Vector3::Zero(),
                Vector3::One() * 3.0f); // 爆発エフェクトを生成
        }
    }

}
