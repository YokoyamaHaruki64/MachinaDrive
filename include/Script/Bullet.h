#pragma once

#include "System/Script.h"

class Bullet : public Script
{
    static constexpr float SPEED = 50.0f;
    static constexpr float HIT_RADIUS = 1.2f; // 当たり判定の半径
    static constexpr float LIFE_TIME = 5.0f; // 弾の寿命（秒）

    Vector3 m_velocity = Vector3::Zero();
    float m_elapsedTime = 0.0f; // 経過時間

public:
    Bullet() = delete;
    explicit Bullet(GameObject* owner)
        : Script(owner)
    {
    }
    void Update() override;

    void SetVelocity(const Vector3& velocity)
    {
        m_velocity = velocity;
    }
};
