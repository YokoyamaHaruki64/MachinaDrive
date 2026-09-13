#pragma once

#include "Component/PostEffect.h"

class GlitchPostProcess final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(GlitchPostProcess, "Glitch", false, true);

public:
    GlitchPostProcess() = delete;
    explicit GlitchPostProcess(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;
};
