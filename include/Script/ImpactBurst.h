#pragma once

#include "System/Component.h"

class ImpactBurst : public Component
{

public:
    ImpactBurst(GameObject* owner) : Component(owner) {}
    virtual ~ImpactBurst() override {};
    virtual void OnStart() override;
};
