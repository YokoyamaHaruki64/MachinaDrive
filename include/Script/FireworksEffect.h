#pragma once

#include "System/Script.h"

class FireworksEffect : public Script
{
    REGISTER_COMPONENT(FireworksEffect, "FireworksEffect");
public:
    FireworksEffect(GameObject* owner) : Script(owner) {}
    virtual ~FireworksEffect() override {};
    virtual void OnStart() override;
    virtual void Update() override;
};
