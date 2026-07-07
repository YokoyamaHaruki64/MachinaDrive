#pragma once

#include "System/Component.h"
#include "math/Vector3.h"

class BoundBall : public Component
{
    static constexpr float GRAVITY = 30.0f; // 重力加速度
    static constexpr int MAX_BOUND_COUNT = 5; // 最大バウンド回数
    Vector3 m_velocity;
    float m_radius = 0.3f; // 球の半径
    float m_bounciness = 0.9f; // 反発係数
    int m_boundCount = 0; // バウンド回数

    void CreateRippleEffect(Vector3 normal, Vector3 position);
public:
    BoundBall(GameObject* owner) : Component(owner), m_velocity(Vector3::Zero()) {}
    virtual ~BoundBall() override {};
    virtual void Update() override;

    void SetVelocity(const Vector3& velocity) { m_velocity = velocity; }
    const Vector3& GetVelocity() const { return m_velocity; }
};
