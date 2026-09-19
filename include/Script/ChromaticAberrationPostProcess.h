#pragma once

#include "Component/PostEffect.h"

class ChromaticAberrationPostProcess final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(ChromaticAberrationPostProcess, "Chromatic Aberration", false, true);

public:
    ChromaticAberrationPostProcess() = delete;
    explicit ChromaticAberrationPostProcess(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;
};
