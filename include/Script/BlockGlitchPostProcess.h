#pragma once

#include "Component/PostEffect.h"

class BlockGlitchPostProcess final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(BlockGlitchPostProcess, "Block Glitch", false, true);

public:
    BlockGlitchPostProcess() = delete;
    explicit BlockGlitchPostProcess(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;
};
