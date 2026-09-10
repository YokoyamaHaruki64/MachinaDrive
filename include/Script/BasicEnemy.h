#pragma once

#include "Script/Enemy.h"

class BasicEnemy final : public Enemy
{
    REGISTER_COMPONENT(BasicEnemy, "Basic Enemy");

protected:
    void RegisterFields(FieldRegistry& fields) override;
    void UpdateBehavior(const Vector3& playerPosition) override;

public:
    BasicEnemy() = delete;
    explicit BasicEnemy(GameObject* owner)
        : Enemy(owner)
    {
    }
};
