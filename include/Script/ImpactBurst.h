#pragma once

#include "System/Script.h"

class ImpactBurst : public Script
{

public:
    ImpactBurst(GameObject* owner) : Script(owner) {}
    virtual ~ImpactBurst() override {};
    virtual void OnStart() override;
};
