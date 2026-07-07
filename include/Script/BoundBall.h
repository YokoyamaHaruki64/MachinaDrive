#pragma once

#include "System/Component.h"
#include "math/Vector3.h"

class BallManager;

class BoundBall : public Component
{
    static constexpr float GRAVITY = 30.0f; // 重力加速度
    static constexpr int MAX_BOUND_COUNT = 10; // 最大バウンド回数

    static constexpr float LIFETIME_MIN = 2; // 最小寿命
    static constexpr float LIFETIME_MAX = 4; // 最大寿命

    Vector3 m_velocity;
    BallManager* m_ballManager = nullptr; // ボールマネージャーへの参照
    float m_radius = 0.3f;      // 球の半径
    float m_bounciness = 1.0f;  // 反発係数
    int m_boundCount = 0;       // バウンド回数

    float m_lifetime = 0.0f;    // 寿命
    float m_elapsedTime = 0.0f; // 経過時間

    void CreateRippleEffect(Vector3 normal, Vector3 position);
public:
    BoundBall(GameObject* owner) : Component(owner), m_velocity(Vector3::Zero()) {}
    virtual ~BoundBall() override {};
    virtual void OnStart() override;
    virtual void Update() override;

    void SetVelocity(const Vector3& velocity) { m_velocity = velocity; }
    const Vector3& GetVelocity() const { return m_velocity; }
};
