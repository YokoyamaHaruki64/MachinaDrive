#pragma once

#include "Component/PostEffect.h"

class NoisePostProcess final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(NoisePostProcess, "Noise", false, true);

public:
    NoisePostProcess() = delete;
    explicit NoisePostProcess(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;
};
