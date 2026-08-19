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

    Material* ResolveMaterial(uint32_t materialSlot) const;

public:
    static void Preload(const char* fileName);
    static void UnloadAll();

    ModelRenderer() = delete;
    ModelRenderer(
        GameObject* object,
        const char* modelFileName,
        uint8_t inverseAxisFlags = 0)
        : RendererBase(object), m_inverseAxisFlags(inverseAxisFlags)
    {
        Load(modelFileName);
    }
    ModelRenderer(
        GameObject* object,
        ModelAsset* modelAsset,
        uint8_t inverseAxisFlags = 0)
        : RendererBase(object), m_inverseAxisFlags(inverseAxisFlags)
    {
        Load(modelAsset);
    }

    void Load(const char* modelFileName);
    void Load(ModelAsset* modelAsset);

    void SetMaterial(uint32_t materialSlot, Material* material);
    Material* GetMaterial(uint32_t materialSlot = 0) override;
    uint32_t GetMeshID() const override;

    void Draw() override;
};
