#pragma once

#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"

#include <vector>

class ModelRenderer;
class ObjectShake;

class Enemy : public Script
{
protected:
    GameObjectReference m_playerObject;
    class Health* m_health{};

    float m_invincibilityDuration = 0.5f;
    float m_invincibilityTimer = 0.0f;
    PrefabReference m_timeRecoveryItemPrefab{ "Assets/Prefabs/TimeRecoveryItem.prefab" };
    float m_hitShakeDuration = 0.2f;
    float m_hitShakePower = 0.2f;
    std::vector<ModelRenderer*> m_modelRenderers;
    ObjectShake* m_objectShake = nullptr;
    bool m_pendingDeath = false;
    bool m_hitFlashActive = false;

    void CacheModelRenderers();
    void SetHitFlash(bool enabled);
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
};
