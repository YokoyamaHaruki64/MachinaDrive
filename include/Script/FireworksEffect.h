#pragma once

#include "System/Component.h"

class FireworksEffect : public Component
{
public:
    FireworksEffect(GameObject* owner) : Component(owner) {}
    virtual ~FireworksEffect() override {};
    virtual void OnStart() override;
    virtual void Update() override;
};
