#pragma once

#include "Component/PostEffect.h"

class RadialBlurPostProcess final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(RadialBlurPostProcess, "Radial Blur", false, true);

public:
    RadialBlurPostProcess() = delete;
    explicit RadialBlurPostProcess(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;
};
