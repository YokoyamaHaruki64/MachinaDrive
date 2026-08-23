#pragma once

#include "System/Script.h"
#include "math/Vector3.h"

class BallManager;
class EffectConfig;

class BoundBall : public Script
{
    static constexpr float RADIUS = 0.3f; // 球の半径
    static constexpr float RIPPLE_EFFECT_COOLDOWN = 0.2f; // Rippleエフェクトのクールダウン時間
    Vector3 m_velocity;
    BallManager* m_ballManager = nullptr; // ボールマネージャーへの参照
    EffectConfig* m_config = nullptr; // エフェクト設定への参照    

    float m_lifetime = 0.0f;    // 寿命
    float m_elapsedTime = 0.0f; // 経過時間
    float m_rippleEffectCooldown = 0.0f; // Rippleエフェクトのクールダウン時間

    void CreateRippleEffect(Vector3 normal, Vector3 position);
public:
    BoundBall(GameObject* owner) : Script(owner), m_velocity(Vector3::Zero()) {}
    virtual ~BoundBall() override {};
    virtual void OnStart() override;
    virtual void Update() override;

    void SetVelocity(const Vector3& velocity) { m_velocity = velocity; }
    const Vector3& GetVelocity() const { return m_velocity; }
};
