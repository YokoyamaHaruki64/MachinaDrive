#pragma once

#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"

class Enemy : public Script
{
protected:
    GameObjectReference m_playerObject;
    class Health* m_health{};

    float m_shootTimer = 0.0f;

    float m_attackRange = 10.0f;
    float m_shootCooldown = 1.5f;
    float m_projectileSpeed = 30.0f;
    PrefabReference m_bulletPrefab{ "Assets/Prefabs/Bullet.prefab" };

    void UpdateShooting(const Vector3& playerPosition, float playerDistanceSqr);
    void ShootAt(const Vector3& targetPosition);
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

};
