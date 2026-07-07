#pragma once

#include "System/Component.h"
#include "math/Vector3.h"

class BoundBall : public Component
{
    Vector3 m_velocity;
    float m_radius = 0.3f; // 球の半径
    float m_bounciness = 1.0f; // 反発係数

    void CreateRippleEffect(Vector3 normal, Vector3 position);
public:
    BoundBall(GameObject* owner) : Component(owner), m_velocity(Vector3::Zero()) {}
    virtual ~BoundBall() override {};
    virtual void Update() override;

    void SetVelocity(const Vector3& velocity) { m_velocity = velocity; }
    const Vector3& GetVelocity() const { return m_velocity; }
};
