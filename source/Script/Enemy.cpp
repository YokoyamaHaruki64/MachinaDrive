#include "pch.h"

#include "Component/ComponentInc.h"

#include "Component/modelRenderer.h"
#include "Component/Transform.h"
#include "Script/Enemy.h"
#include "Script/Health.h"
#include "Script/Player.h"
#include "Script/PostProcessImpulse.h"
#include "Script/Score.h"
#include "Script/ObjectShake.h"
#include "Script/UI/GameHud.h"
#include "System/Editor/FieldRegistry.h"

#include <cmath>

void Enemy::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_invincibilityDuration);
    GM_FIELD(m_timeRecoveryItemPrefab);
    GM_FIELD(m_enemyDeathVfxPrefab);
    GM_FIELD(m_hitShakeDuration);
    GM_FIELD(m_hitShakePower);
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

    m_invincibilityTimer = 0.0f;
    m_pendingDeath = false;
    m_hitFlashActive = false;
    m_objectShake = nullptr;
    CacheModelRenderers();

    m_health->OnDeath.emplace_back([this]()
    {
        if (!m_owner->IsDestroyed())
        {
            // PlayerHitのHitStop開始はTakeDamageの後段で行われるため、ここでは死亡を保留する。
            m_pendingDeath = true;
        }
    });
}

void Enemy::Update()
{
    m_invincibilityTimer = Math::Max(m_invincibilityTimer - Time::GetDeltaTime(), 0.0f);

    Player* player = m_playerObject ? m_playerObject->GetComponent<Player>() : nullptr;
    if (m_pendingDeath)
    {
        if (player && player->IsHitStop())
        {
            return;
        }

        FinalizeDeath();
        return;
    }

    if (m_hitFlashActive && (!player || !player->IsHitStop()))
    {
        SetHitFlash(false);
    }

    if (m_playerObject && m_playerObject->GetTransform())
    {
        const Vector3 playerPosition = m_playerObject->GetTransform()->GetPosition();
        FacePlayer(playerPosition);
        UpdateBehavior(playerPosition);
    }
}

void Enemy::FacePlayer(const Vector3& playerPosition)
{
    Transform* enemyTransform = transform();
    if (!enemyTransform)
    {
        return;
    }

    Vector3 direction = playerPosition - enemyTransform->GetPosition();
    direction.y = 0.0f;
    if (direction.LengthSqr() <= 1.0e-6f)
    {
        // 同じ位置では向きが定まらないため、現在の回転を維持する。
        return;
    }

    enemyTransform->SetRotation(Quaternion::LookRotation(direction));
}

bool Enemy::TakeDamage(float damage)
{
    if (!m_health || m_health->IsDead() || m_owner->IsDestroyed() || damage <= 0.0f || m_invincibilityTimer > 0.0f)
    {
        return false;
    }

    m_invincibilityTimer = Math::Max(m_invincibilityDuration, 0.0f);
    SetHitFlash(true);
    if (m_objectShake)
    {
        m_objectShake->Shake(m_hitShakeDuration, m_hitShakePower);
    }
    m_health->TakeDamage(damage);
    PostProcessImpulse::Play("EnemyHit");
    return true;
}

void Enemy::CacheModelRenderers()
{
    m_modelRenderers.clear();

    Transform* enemyTransform = transform();
    if (!enemyTransform)
    {
        return;
    }

    for (TransformBase* childTransform : enemyTransform->GetChildren())
    {
        GameObject* childObject = childTransform ? childTransform->GetOwner() : nullptr;
        if (!m_objectShake && childObject)
        {
            m_objectShake = childObject->GetComponent<ObjectShake>();
        }

        if (ModelRenderer* renderer = childObject ? childObject->GetComponent<ModelRenderer>() : nullptr)
        {
            m_modelRenderers.push_back(renderer);
        }
    }
}

void Enemy::SetHitFlash(bool enabled)
{
    for (ModelRenderer* renderer : m_modelRenderers)
    {
        if (!renderer)
        {
            continue;
        }

        for (uint32_t slot = 0; slot < renderer->GetMaterialSlotCount(); ++slot)
        {
            MaterialPropertyOverride& properties = renderer->GetMaterialPropertyOverride(slot);
            if (enabled)
            {
                properties.SetColor("_BaseColor", Color::White());
                properties.SetColor("_Emission", Color::White());
            }
            else
            {
                renderer->ClearMaterialPropertyOverride(slot);
            }
        }
    }

    m_hitFlashActive = enabled;
}

void Enemy::FinalizeDeath()
{
    if (m_owner->IsDestroyed())
    {
        return;
    }

    m_pendingDeath = false;
    if (m_enemyDeathVfxPrefab)
    {
        Manager::Instantiate(
            m_enemyDeathVfxPrefab.GetPath(),
            transform()->GetPosition() + Vector3::Up(),
            Vector3::Zero(),
            Vector3::One());
    }

    DropTimeRecoveryItem();
    m_owner->Destroy();
}

void Enemy::DropTimeRecoveryItem()
{
    if (!m_timeRecoveryItemPrefab)
    {
        return;
    }

    Manager::Instantiate(
        m_timeRecoveryItemPrefab.GetPath(),
        transform()->GetPosition() + Vector3(0.0f, 1.5f, 0.0f),
        Vector3::Zero(),
        Vector3::One());
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
    if (GameObject* hudObject = Manager::GetFirstObjectByComponent<GameHud>())
    {
        if (GameHud* gameHud = hudObject->GetComponent<GameHud>())
        {
            gameHud->SetEnemyCount(static_cast<int>(Manager::GetObjectsByTag("Enemy").size()));
        }
    }

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
