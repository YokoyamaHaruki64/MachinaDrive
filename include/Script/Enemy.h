#pragma once

#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"

#include <vector>

class ModelRenderer;
class ObjectShake;
class Collider;
class Rigidbody;

class Enemy : public Script
{
public:
    enum class Phase
    {
        Active,
        HitStop,
        KnockBack,
        Dead,
    };

protected:
    GameObjectReference m_playerObject;
    class Health* m_health{};
    Rigidbody* m_enemyRigidbody = nullptr;
    Collider* m_collider = nullptr;

    float m_invincibilityDuration = 0.5f;
    float m_invincibilityTimer = 0.0f;
    PrefabReference m_timeRecoveryItemPrefab{ "Assets/Prefabs/TimeRecoveryItem.prefab" };
    PrefabReference m_enemyDeathVfxPrefab{ "Assets/Prefabs/VFX_EnemyDeath.prefab" };
    float m_hitShakeDuration = 0.2f;
    float m_hitShakePower = 0.2f;
    float m_knockbackDistance = 3.0f;
    float m_knockbackDuration = 0.25f;
    float m_knockbackAngle = 30.0f;
    float m_knockbackUpwardSpeed = 3.0f;
    std::vector<ModelRenderer*> m_modelRenderers;
    ObjectShake* m_objectShake = nullptr;
    bool m_hitFlashActive = false;
    float m_knockbackTimer = 0.0f;
    Vector3 m_knockbackDirection = Vector3::Forward();
    Phase m_phase = Phase::Active;

    void CacheModelRenderers();
    void SetHitFlash(bool enabled);
    void BeginHitStop();
    void BeginKnockBack();
    void UpdateKnockBack();
    void FinalizeDeath();
    void DropTimeRecoveryItem();
    void FacePlayer(const Vector3& playerPosition);
    virtual void UpdateBehavior(const Vector3& playerPosition) = 0;

    explicit Enemy(GameObject* owner)
        : Script(owner)
    {
    }

    void RegisterFields(FieldRegistry& fields) override;

public:
    void OnStart() override;
    void Update() final override;
    void OnHitBegin(const HitInfo& hitInfo) override;
    void OnDestroy() override;

    bool TakeDamage(float damage);
    Phase GetPhase() const { return m_phase; }
};
