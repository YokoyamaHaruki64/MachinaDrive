#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/Bullet.h"
#include "Script/Enemy.h"
#include "Script/Health.h"
#include "Script/Score.h"
#include "System/Editor/FieldRegistry.h"

#include <cmath>

void Enemy::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_attackRange);
    GM_FIELD(m_shootCooldown);
    GM_FIELD(m_projectileSpeed);
    GM_FIELD(m_bulletPrefab);
}

void Enemy::OnStart()
{
    m_playerObject = Manager::GetObjectFirstByTag("Player");
    m_health = GetComponent<Health>();
    if (!m_health)
    {
        Logger::Error("Enemy: Health component not found.");
        return;
    }

    m_health->OnDeath.emplace_back([this]()
    {
        if (!m_owner->IsDestroyed())
        {
            m_owner->Destroy();
        }
    });
}

void Enemy::Update()
{
    const Vector3 position = transform()->GetPosition();
    if (m_playerObject && m_playerObject->GetTransform())
    {
        const Vector3 playerPosition = m_playerObject->GetTransform()->GetPosition();
        Vector3 toPlayer = playerPosition - position;
        toPlayer.y = 0.0f;
        const float playerDistanceSqr = toPlayer.LengthSqr();

        UpdateBehavior(playerPosition);
        UpdateShooting(playerPosition, playerDistanceSqr);
    }
}

void Enemy::UpdateShooting(const Vector3& playerPosition, float playerDistanceSqr)
{
    if (playerDistanceSqr > m_attackRange * m_attackRange)
    {
        return;
    }

    m_shootTimer -= Time::GetDeltaTime();
    if (m_shootTimer <= 0.0f)
    {
        ShootAt(playerPosition);
        m_shootTimer = Math::Max(m_shootCooldown, 0.01f);
    }
}

void Enemy::ShootAt(const Vector3& targetPosition)
{
    Vector3 direction = targetPosition - transform()->GetPosition();
    direction.y = 0.0f;
    if (direction.LengthSqr() <= 0.0f)
    {
        return;
    }

    direction.Normalize();
    if (!m_bulletPrefab)
    {
        return;
    }

    GameObject* projectileObject = Manager::Instantiate(
        m_bulletPrefab.GetPath(),
        transform()->GetPosition() + direction * 0.8f + Vector3(0.0f, 1.0f, 0.0f),
        Vector3::Zero(),
        Vector3::One());
    if (projectileObject)
    {
        if (Bullet* projectile = projectileObject->GetComponent<Bullet>())
        {
            projectile->SetVelocity(direction * m_projectileSpeed);
            projectile->SetEnemyProjectile(true);
        }
    }
}

void Enemy::OnHitBegin(const HitInfo& hitInfo)
{
    if (hitInfo.objectA != m_owner || !hitInfo.objectB || hitInfo.objectB->GetTag() != "Wall")
    {
        return;
    }

}

void Enemy::OnDestroy()
{
    auto obj = Manager::GetFirstObjectByComponent<Score>();
    if (obj)
    {
        Score* scoreComponent = obj->GetComponent<Score>();
        if (scoreComponent)
        {
            scoreComponent->AddScore(75);
        }
    }
}
