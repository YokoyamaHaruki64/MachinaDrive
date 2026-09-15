#pragma once

#include "System/Core/Script.h"

// 同一GameObjectで生存するComponentが、被ダメージと死亡へ反応するための共通Script。
class Health final : public Script
{
    GM_SCRIPT(Health, "Health");
    USE_FIELDS

public:
    using DamageCallback = std::function<void(float)>;
    using DeathCallback = std::function<void()>;

    // 登録先はHealthと同じGameObjectにあり、Healthの寿命中に破棄されないComponentに限る。
    std::vector<DamageCallback> OnDamage;
    std::vector<DeathCallback> OnDeath;

private:
    float m_maxHP{ 1.0f };
    float m_currentHP{ 1.0f };
    bool m_isDead{ false };

public:
    void OnStart() override;

    void TakeDamage(float damage);

    float GetMaxHP() const { return m_maxHP; }
    float GetCurrentHP() const { return m_currentHP; }
    bool IsDead() const { return m_isDead; }
};
