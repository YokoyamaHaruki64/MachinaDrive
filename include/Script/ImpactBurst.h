#pragma once

#include "System/Script.h"

class ImpactBurst : public Script
{
    REGISTER_COMPONENT(ImpactBurst, "ImpactBurst");

public:
    ImpactBurst(GameObject* owner) : Script(owner) {}
    virtual ~ImpactBurst() override {};
    virtual void OnStart() override;
};
