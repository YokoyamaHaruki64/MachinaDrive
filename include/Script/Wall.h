#pragma once

#include "System/Component.h"

class Wall : public Component
{
public:
    Wall() = delete;
    explicit Wall(GameObject* owner)
        : Component(owner)
    {
    }
    virtual ~Wall() {}
    virtual void Update() override {};
};
