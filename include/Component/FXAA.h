#pragma once

#include "Component/PostEffect.h"

class FXAA final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(FXAA, "FXAA", true, true);

public:
    FXAA() = delete;
    explicit FXAA(GameObject* owner);
    void Apply() override;
};
