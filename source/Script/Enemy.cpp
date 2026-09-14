#include "pch.h"

#include "Component/ComponentInc.h"

#include "Common/Easing.h"

#include "Common/Random.h"

#include "Component/Collider.h"
#include "Component/modelRenderer.h"
#include "Component/Rigidbody.h"
#include "Component/Transform.h"
#include "Script/Enemy.h"
#include "Script/Health.h"
#include "Script/Player.h"
#include "Script/PostProcessImpulse.h"
#include "Script/Score.h"
#include "Script/ObjectShake.h"
#include "Script/UI/GameHud.h"
#include "Script/WaveDirector.h"
#include "System/Editor/FieldRegistry.h"

#include <cmath>

void Enemy::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_invincibilityDuration);
    GM_FIELD(m_timeRecoveryItemPrefab);
    GM_FIELD(m_enemyDeathVfxPrefab);
    GM_FIELD(m_hitShakeDuration);
    GM_FIELD(m_hitShakePower);
    GM_FIELD(m_knockbackDistance);
    GM_FIELD(m_knockbackDuration);
    GM_FIELD(m_knockbackAngle);
    GM_FIELD(m_knockbackUpwardSpeed);
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
    m_hitFlashActive = false;
    m_knockbackTimer = 0.0f;
    m_phase = Phase::Active;
    m_enemyRigidbody = GetComponent<Rigidbody>();
    m_collider = GetComponent<Collider>();
    m_objectShake = nullptr;
    CacheModelRenderers();

    m_health->OnDeath.emplace_back([this]()
    {
        if (!m_owner->IsDestroyed())
        {
            BeginHitStop();
            SetHitFlash(true);
        }
    });
}

void Enemy::Update()
{
    m_invincibilityTimer = Math::Max(m_invincibilityTimer - Time::GetDeltaTime(), 0.0f);
    Player* player = m_playerObject ? m_playerObject->GetComponent<Player>() : nullptr;

    if (m_phase == Phase::HitStop)
    {
        if (player && player->IsHitStop())
        {
            return;
        }

        BeginKnockBack();
        return;
    }

    if (m_phase == Phase::KnockBack)
    {
        UpdateKnockBack();
        return;
    }

    if (m_phase == Phase::Dead)
    {
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

void Enemy::BeginHitStop()
{
    m_phase = Phase::HitStop;
    if (m_enemyRigidbody)
    {
        m_enemyRigidbody->SetVelocity(Vector3::Zero());
    }
}

bool Enemy::TakeDamage(float damage)
{
    if (m_phase != Phase::Active
        || !m_health
        || m_health->IsDead()
        || m_owner->IsDestroyed()
        || damage <= 0.0f
        || m_invincibilityTimer > 0.0f)
    {
        return false;
    }

    m_invincibilityTimer = Math::Max(m_invincibilityDuration, 0.0f);
    SetHitFlash(true);
    BeginHitStop();
    if (m_objectShake)
    {
        m_objectShake->Shake(m_hitShakeDuration, m_hitShakePower);
    }
    m_health->TakeDamage(damage);
    PostProcessImpulse::Play("EnemyHit");
    return true;
}

void Enemy::BeginKnockBack()
{
    if (m_phase != Phase::HitStop)
        return;

    m_phase = Phase::KnockBack;
    m_knockbackDistance = Math::Max(m_knockbackDistance, 0.0f);
    m_knockbackDuration = Math::Max(m_knockbackDuration, 0.0f);
    m_knockbackUpwardSpeed = Math::Max(m_knockbackUpwardSpeed, 0.0f);
    m_knockbackTimer = m_knockbackDuration;
    SetHitFlash(true);

    if (m_collider)
    {
        m_collider->SetEnabled(false);
    }

    Vector3 direction = Vector3::Forward();
    if (m_playerObject)
    {
        if (Rigidbody* playerRigidbody = m_playerObject->GetComponent<Rigidbody>())
        {
            direction = playerRigidbody->GetVelocity();
        }

        direction.y = 0.0f;
    }

    if (direction.LengthSqr() <= 1.0e-6f && m_playerObject && m_playerObject->GetTransform())
    {
        direction = m_playerObject->GetTransform()->GetForward();
        direction.y = 0.0f;
    }

    if (direction.LengthSqr() <= 1.0e-6f)
    {
        direction = Vector3::Forward();
    }

    direction.Normalize();

    float angleSign = 1.0f;
    if (m_playerObject && m_playerObject->GetTransform() && transform())
    {
        Vector3 toEnemy = transform()->GetPosition() - m_playerObject->GetTransform()->GetPosition();
        toEnemy.y = 0.0f;

        Vector3 playerRight = Vector3::Cross(Vector3::Up(), direction);
        playerRight.Normalize();

        const float side = Vector3::Dot(toEnemy, playerRight);
        if (std::abs(side) > 1.0e-4f)
        {
            angleSign = side < 0.0f ? -1.0f : 1.0f;
        }
    }

    m_knockbackDirection = Quaternion::FromAxisAngle(
        Vector3::Up(),
        m_knockbackAngle * angleSign * Math::DEG2RAD).RotateVector(direction);

    m_knockbackDirection.y = 0.0f;
    m_knockbackDirection.Normalize();

    if (m_enemyRigidbody)
    {
        const float speed = m_knockbackDuration > 0.0f
            ? m_knockbackDistance / m_knockbackDuration * 3.0f
            : 0.0f;
        Vector3 velocity = m_knockbackDirection * speed;
        velocity.y = m_knockbackUpwardSpeed;
        m_enemyRigidbody->SetVelocity(velocity);
    }
}

void Enemy::UpdateKnockBack()
{
    m_knockbackTimer = Math::Max(m_knockbackTimer - Time::GetDeltaTime(), 0.0f);

    if (m_enemyRigidbody && m_knockbackDuration > 0.0f)
    {
        const float progress = Math::Clamp(
            1.0f - m_knockbackTimer / m_knockbackDuration,
            0.0f,
            1.0f);
        const float speedProgress = 1.0f - Easing::Evaluate(progress, EaseType::OutQuad);
        const float speed = m_knockbackDistance / m_knockbackDuration * 3.0f * speedProgress;
        Vector3 velocity = m_knockbackDirection * speed;
        velocity.y = m_knockbackUpwardSpeed * (1.0f - progress * 2.0f);
        m_enemyRigidbody->SetVelocity(velocity);
    }

    if (m_knockbackTimer > 0.0f)
    {
        return;
    }

    if (m_enemyRigidbody)
    {
        m_enemyRigidbody->SetVelocity(Vector3::Zero());
    }

    if (m_health && m_health->IsDead())
    {
        FinalizeDeath();
        return;
    }

    m_phase = Phase::Active;
    SetHitFlash(false);
    if (m_collider)
    {
        m_collider->SetEnabled(true);
    }
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

    m_phase = Phase::Dead;
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

    float dropRate = 1.0f;
    if (GameObject* directorObject = Manager::GetFirstObjectByComponent<WaveDirector>())
    {
        dropRate = directorObject->GetComponent<WaveDirector>()->GetTimeRecoveryDropRate();
    }

    dropRate = Math::Clamp(dropRate, 0.0f, 1.0f);
    if (dropRate <= 0.0f
        || (dropRate < 1.0f && Random::Range(0.0f, 1.0f) >= dropRate))
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
