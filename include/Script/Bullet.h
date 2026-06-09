#pragma once

#include "System/component.h"

class Bullet : public Component
{
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
