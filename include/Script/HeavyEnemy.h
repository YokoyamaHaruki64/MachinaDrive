#pragma once

#include "Script/Enemy.h"

class HeavyEnemy final : public Enemy
{
    REGISTER_COMPONENT(HeavyEnemy, "Heavy Enemy");

protected:
    void RegisterFields(FieldRegistry& fields) override;
    void UpdateBehavior(const Vector3& playerPosition) override;

public:
    HeavyEnemy() = delete;
    explicit HeavyEnemy(GameObject* owner)
        : Enemy(owner)
    {
    }
};
