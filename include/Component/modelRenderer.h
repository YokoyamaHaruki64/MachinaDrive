#pragma once

#include "Graphics/ModelAsset.h"
#include "System/Core/ComponentRegistry.h"
#include "System/Rendering/RendererBase.h"

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
    std::string m_loadedModelPath;
    std::vector<Material*> m_materialOverrides{};
    std::vector<std::string> m_materialPaths{};
    int m_inverseAxisFlags{};
    bool m_castShadow = true;
    bool m_receiveShadow = true;

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

    bool GetCastShadow() const { return m_castShadow; }
    void SetCastShadow(bool value) { m_castShadow = value; }
    bool GetReceiveShadow() const { return m_receiveShadow; }
    void SetReceiveShadow(bool value) { m_receiveShadow = value; }

    void Draw() override;
    void Submit() override;
    void OnLoaded() override;
    void OnEditorUpdate() override;

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
