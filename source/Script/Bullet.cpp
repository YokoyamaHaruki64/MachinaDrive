#include "pch.h"

#include "Component/ComponentInc.h"


#include "Script/Bullet.h"
#include "Script/Enemy.h"

void Bullet::Update()
{
    transform()->SetPosition(transform()->GetPosition() + m_velocity * Time::GetDeltaTime());
    m_elapsedTime += Time::GetDeltaTime();
    if (m_elapsedTime >= LIFE_TIME)
    {
        m_owner->Destroy(); // 寿命が尽きたら弾を破壊
        return;
    }

    // 当たり判定
    auto enemies = Manager::GetObjectsByComponent<Enemy>();
    for( auto& enemyObj : enemies)
    {
        Transform* enemyTransform = enemyObj->GetTransform();
        // 簡易的な当たり判定（距離が一定以下ならヒットとする
        float distance =
            (transform()->GetPosition() - enemyTransform->GetPosition()).LengthSqr();
        if (distance < HIT_RADIUS * HIT_RADIUS) // ヒットの閾値
        {
            enemyObj->Destroy();    // 敵を破壊
            m_owner->Destroy();     // 弾を破壊
            Manager::InstantiatePrefab(
                "Assets/Prefabs/Explosion.prefab",
                enemyTransform->GetPosition() + Vector3(0.0f, 0.5f, 0.0f), // 敵の位置に少し上げてエフェクトを生成
                Vector3::Zero(),
                Vector3::One() * 3.0f); // 爆発エフェクトを生成
        }
    }

}
