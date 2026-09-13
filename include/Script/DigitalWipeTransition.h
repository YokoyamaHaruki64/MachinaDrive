#pragma once

#include "Component/PostEffect.h"

class DigitalWipeTransition final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(DigitalWipeTransition, "Digital Wipe Transition", false, true);

public:
    DigitalWipeTransition() = delete;
    explicit DigitalWipeTransition(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;
};
