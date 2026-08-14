#pragma once

#include "Graphics/ModelAsset.h"
#include "System/RendererBase.h"

#include <string>
#include <unordered_map>

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
    static std::unordered_map<std::string, ModelAsset*> m_ModelPool;

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
        const char* vertexShaderFileName,
        const char* pixelShaderFileName,
        uint8_t inverseAxisFlags = 0)
        : RendererBase(object), m_inverseAxisFlags(inverseAxisFlags)
    {
        // 旧Shader指定は、ModelAssetが所有するMaterialへの完全移行までAPI互換のため受け取る。
        Load(modelFileName, vertexShaderFileName, pixelShaderFileName);
    }

    void Load(
        const char* modelFileName,
        const char* vertexShaderFileName,
        const char* pixelShaderFileName);

    void SetMaterial(uint32_t materialSlot, Material* material);
    Material* GetMaterial(uint32_t materialSlot) const;

    void Draw() override;
};
