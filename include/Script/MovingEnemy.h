#pragma once

#include "Script/Enemy.h"

class MovingEnemy final : public Enemy
{
    REGISTER_COMPONENT(MovingEnemy, "Moving Enemy");

    float m_teleportTimer = 0.0f;
    float m_teleportInterval = 3.0f;
    float m_teleportDistance = 8.0f;

    static constexpr float MAX_TELEPORT_DISTANCE_FROM_ORIGIN = 45.0f;

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
