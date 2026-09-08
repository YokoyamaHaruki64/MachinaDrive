#pragma once

#include "Component/PostEffect.h"

class Bloom final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(Bloom, "Bloom", true, true);

    TextureHandle m_bloomTexture{};

    size_t m_bloomExtractPassIndex{};
    size_t m_compositePassIndex{};

public:
    Bloom() = delete;
    explicit Bloom(GameObject* owner);
    void Resize(UINT width, UINT height) override;
    void Apply() override;

protected:
    void OnMaterialChanged() override;
};
