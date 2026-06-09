#pragma once

#include "System/component.h"

class Bullet : public Component
{
    static constexpr float Speed = 50.0f;
    static constexpr float HitRadius = 1.2f; // 当たり判定の半径
    Vector3 m_velocity = Vector3::Zero();

public:
    Bullet() = delete;
    explicit Bullet(GameObject* owner)
        : Component(owner)
    {
    }
    void Update() override;

    void SetVelocity(const Vector3& velocity)
    {
        m_velocity = velocity;
    }
};
