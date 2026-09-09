#include "pch.h"

#include "Component/ComponentInc.h"


#include "Script/Bullet.h"
#include "Script/Enemy.h"
#include "Script/Player.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(Bullet)
    GM_FIELD(m_trailWidthCurve);
    GM_FIELD(m_trailColorCurve);
FIELDS_END()

void Bullet::OnStart()
{
    m_trailRenderer = GetComponent<TrailRenderer>();
    ApplyTrailSettings();
}

void Bullet::ApplyTrailSettings()
{
    if (!m_trailRenderer)
    {
        return;
    }

    m_trailRenderer->SetWidthCurve(m_trailWidthCurve);
    m_trailRenderer->SetColorCurve(m_trailColorCurve);
}

void Bullet::SetTrailWidthCurve(const Curve<float>& curve)
{
    m_trailWidthCurve = curve;
    ApplyTrailSettings();
}

void Bullet::SetTrailColorCurve(const CurveColor& curve)
{
    m_trailColorCurve = curve;
    ApplyTrailSettings();
}

void Bullet::Update()
{
    transform()->SetPosition(transform()->GetPosition() + m_velocity * Time::GetDeltaTime());
    m_elapsedTime += Time::GetDeltaTime();
    if (m_elapsedTime >= LIFE_TIME)
    {
        m_owner->Destroy(); // 寿命が尽きたら弾を破壊
        return;
    }

}

void Bullet::OnHitBegin(const HitInfo& hitInfo)
{
    if (hitInfo.objectA != m_owner || !hitInfo.objectB)
    {
        return;
    }

    if (m_isEnemyProjectile)
    {
        if (hitInfo.objectB->GetTag() == "Player")
        {
            if (Player* player = hitInfo.objectB->GetComponent<Player>())
            {
                player->TakeDamage(10.0f);
            }
            m_owner->Destroy();
        }
        else if (hitInfo.objectB->GetTag() == "Wall")
        {
            m_owner->Destroy();
        }
        return;
    }

    if (Enemy* enemy = hitInfo.objectB->GetComponent<Enemy>())
    {
        enemy->TakeDamage(1);
        m_owner->Destroy();
        Manager::Instantiate(
            "Assets/Prefabs/Explosion.prefab",
            hitInfo.objectB->GetTransform()->GetPosition() + Vector3(0.0f, 0.5f, 0.0f),
            Vector3::Zero(),
            Vector3::One() * 3.0f);
    }
    else if (hitInfo.objectB->GetTag() == "Wall")
    {
        m_owner->Destroy();
    }
}
