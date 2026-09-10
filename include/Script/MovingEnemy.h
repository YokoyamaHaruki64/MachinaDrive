#pragma once

#include "Script/Enemy.h"

class Rigidbody;

class MovingEnemy final : public Enemy
{
    REGISTER_COMPONENT(MovingEnemy, "Moving Enemy");

    Rigidbody* m_rigidbody{ nullptr };

    float m_stateTimer = 0.0f;
    float m_moveDuration = 3.0f;
    float m_stopDuration = 1.0f;
    float m_moveSpeed = 12.0f;
    bool m_isMoving = true;

    static constexpr float STOP_DISTANCE_FROM_PLAYER = 10.0f;

    void SetVelocityTowardsPlayer(const Vector3& playerPosition);
    void BeginMoving(const Vector3& playerPosition);
    void BeginStopping();

protected:
    void RegisterFields(FieldRegistry& fields) override;
    void UpdateBehavior(const Vector3& playerPosition) override;

public:
    MovingEnemy() = delete;
    explicit MovingEnemy(GameObject* owner)
        : Enemy(owner)
    {
    }

    void OnStart() override;
};
