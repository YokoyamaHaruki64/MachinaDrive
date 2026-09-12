#pragma once

#include "Script/Enemy.h"

class Rigidbody;

class MovingEnemy final : public Enemy
{
    REGISTER_COMPONENT(MovingEnemy, "Moving Enemy");

    Rigidbody* m_rigidbody{ nullptr };

    Vector3 m_moveDirection{ Vector3::Right() };
    float m_amplitudeMin = 5.0f;
    float m_amplitudeMax = 10.0f;
    float m_frequencyMin = 0.3f;
    float m_frequencyMax = 0.8f;
    float m_amplitude = 0.0f;
    float m_frequency = 0.0f;
    float m_elapsedTime = 0.0f;

    void SelectMovementParameters();

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
