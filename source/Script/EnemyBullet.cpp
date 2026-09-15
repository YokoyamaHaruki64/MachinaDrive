#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/EnemyBullet.h"

#include "Script/Health.h"

FIELDS_BEGIN(EnemyBullet)
    GM_FIELD(m_trailWidthCurve);
    GM_FIELD(m_trailColorCurve);
FIELDS_END()

void EnemyBullet::OnStart()
{
    m_trailRenderer = GetComponent<TrailRenderer>();
    ApplyTrailSettings();
}

void EnemyBullet::ApplyTrailSettings()
{
    if (!m_trailRenderer)
    {
        return;
    }

    m_trailRenderer->SetWidthCurve(m_trailWidthCurve);
    m_trailRenderer->SetColorCurve(m_trailColorCurve);
}

void EnemyBullet::SetTrailWidthCurve(const Curve<float>& curve)
{
    m_trailWidthCurve = curve;
    ApplyTrailSettings();
}

void EnemyBullet::SetTrailColorCurve(const CurveColor& curve)
{
    m_trailColorCurve = curve;
    ApplyTrailSettings();
}

void EnemyBullet::Update()
{
    transform()->SetPosition(transform()->GetPosition() + m_velocity * Time::GetDeltaTime());
    m_elapsedTime += Time::GetDeltaTime();
    if (m_elapsedTime >= LIFE_TIME)
    {
        m_owner->Destroy(); // 寿命が尽きたら弾を破壊
        return;
    }

}

void EnemyBullet::OnHitBegin(const HitInfo& hitInfo)
{
    if (hitInfo.objectA != m_owner || !hitInfo.objectB)
    {
        return;
    }

    if (hitInfo.objectB->GetTag() == "Player")
    {
        if (Health* health = hitInfo.objectB->GetComponent<Health>())
        {
            health->TakeDamage(10.0f);
        }
        m_owner->Destroy();
        return;
    }

    if (hitInfo.objectB->GetTag() == "Wall")
    {
        m_owner->Destroy();
    }
}
