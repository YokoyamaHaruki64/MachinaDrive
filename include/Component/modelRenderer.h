#pragma once

#include "Graphics/ModelAsset.h"
#include "System/ComponentRegistry.h"
#include "System/RendererBase.h"

#include <string>

class ModelRenderer : public RendererBase
{
    REGISTER_COMPONENT(ModelRenderer, "Model Renderer");

public:
    enum InverseAxis
    {
        X = 1,
        Y = 1 << 1,
        Z = 1 << 2
    };

private:
    ModelAsset* m_model{};
    std::string m_modelPath;
    std::vector<Material*> m_materialOverrides{};
    std::vector<std::string> m_materialPaths{};
    int m_inverseAxisFlags{};

    void InitializeMaterialOverrides();
    Material* ResolveMaterial(uint32_t materialSlot) const;

public:
    ModelRenderer() = delete;
    explicit ModelRenderer(GameObject* object);
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

    void SetMaterial(uint32_t materialSlot, Material* material) override;
    Material* GetMaterial(uint32_t materialSlot = 0) override;
    uint32_t GetMaterialSlotCount() const override { return static_cast<uint32_t>(m_materialOverrides.size()); }
    uint32_t GetMeshID() const override;

    void Draw() override;
    void OnStart() override;

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
