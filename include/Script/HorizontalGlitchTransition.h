#pragma once

#include "Component/PostEffect.h"

class HorizontalGlitchTransition final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(HorizontalGlitchTransition, "Horizontal Glitch Transition", false, true);

public:
    HorizontalGlitchTransition() = delete;
    explicit HorizontalGlitchTransition(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;
};
