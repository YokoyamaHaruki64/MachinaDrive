#pragma once

#include "Component/PostEffect.h"

// 画面全体をカラーからグレースケールへ補間するPostProcess。
class GrayScalePostProcess final : public PostProcess
{
    REGISTER_COMPONENT_OPTIONS(GrayScalePostProcess, "Gray Scale", false, true);

    float m_intensity{};

public:
    GrayScalePostProcess() = delete;
    explicit GrayScalePostProcess(GameObject* owner) : PostProcess(owner) {}

    void Apply() override;

    void SetIntensity(float intensity);
    float GetIntensity() const { return m_intensity; }
};
