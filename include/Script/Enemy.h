#pragma once

#include "System/Script.h"

class Enemy : public Script
{
    REGISTER_COMPONENT(Enemy, "Enemy");
public:
    Enemy() = default;
    explicit Enemy(GameObject* owner)
        : Script(owner)
    {
    }
    void Update() override;
    void OnDestroy() override;
};
