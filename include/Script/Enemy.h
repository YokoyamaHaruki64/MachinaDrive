#pragma once

#include "System/Component.h"

class Enemy : public Component
{
public:
    Enemy() = default;
    explicit Enemy(GameObject* owner)
        : Component(owner)
    {
    }
    void Update() override;
};
