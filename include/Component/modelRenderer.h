#pragma once

#include "Graphics/ModelAsset.h"
#include "System/RendererBase.h"

#include <string>

class ModelRenderer : public RendererBase
{
public:
    enum InverseAxis
    {
        X = 1,
        Y = 1 << 1,
        Z = 1 << 2
    };

private:
    ModelAsset* m_model{};
    std::vector<Material*> m_materialOverrides{};
    uint8_t m_inverseAxisFlags{};

    void InitializeMaterialOverrides();
    Material* ResolveMaterial(uint32_t materialSlot) const;

public:
    ModelRenderer() = delete;
    ModelRenderer(
        GameObject* object,
        const char* modelFileName,
        uint8_t inverseAxisFlags = 0);
    ModelRenderer(
        GameObject* object,
        ModelAsset* modelAsset,
        uint8_t inverseAxisFlags = 0);

    ModelAsset* GetModel() const { return m_model; }
    void SetModel(ModelAsset* modelAsset);

    void SetMaterial(uint32_t materialSlot, Material* material);
    Material* GetMaterial(uint32_t materialSlot = 0) override;
    uint32_t GetMaterialSlotCount() const override { return static_cast<uint32_t>(m_materialOverrides.size()); }
    uint32_t GetMeshID() const override;

    void Draw() override;
};
