#pragma once

#include "Graphics/Material.h"
#include "Graphics/ModelAsset.h"
#include "System/Rendering/RendererBase.h"

class SpriteRenderer : public RendererBase
{
    REGISTER_COMPONENT(SpriteRenderer, "Sprite Renderer");

protected:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_colorBuffer;

    Material* m_material{};
    std::string m_materialPath{ "Assets/Materials/Sprite.material" };

public:
    SpriteRenderer() = delete;
    explicit SpriteRenderer(GameObject* owner);

    virtual ~SpriteRenderer() {}
    void OnStart() override;
    void Draw() override;
    Material* GetMaterial(uint32_t slot = 0) override;
    void SetMaterial(uint32_t slot, Material* material) override;

    void SetUVRect(float offsetX, float offsetY, float width, float height)
    {
        MaterialPropertyOverride& propertyOverride = GetMaterialPropertyOverride();
        propertyOverride.SetVector2("_UVOffset", Vector2(offsetX, offsetY));
        propertyOverride.SetVector2("_UVScale", Vector2(width, height));
    }

    void SetUVOffset(float offsetX, float offsetY)
    {
        GetMaterialPropertyOverride().SetVector2("_UVOffset", Vector2(offsetX, offsetY));
    }

    void SetUVSize(float width, float height)
    {
        GetMaterialPropertyOverride().SetVector2("_UVScale", Vector2(width, height));
    }

    void SetColor(const Color& color)
    {
        GetMaterialPropertyOverride().SetColor("_BaseColor", color);
    }

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
