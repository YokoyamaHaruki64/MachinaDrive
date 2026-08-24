#pragma once

#include "Graphics/Material.h"
#include "Graphics/ModelAsset.h"
#include "System/RendererBase.h"

class SpriteRenderer : public RendererBase
{
    REGISTER_COMPONENT(SpriteRenderer, "Sprite Renderer");

protected:
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_colorBuffer;

    Material* m_materialInstance{};
    TextureHandle m_texture{};
    Color m_baseColor{ Color::White() };
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    std::string m_texturePath;
    std::string m_materialPath{ "Assets/Materials/UnlitTransparent.material" };

    float m_uvOffsetX = 0.0f;
    float m_uvOffsetY = 0.0f;
    float m_uvWidth = 1.0f;
    float m_uvHeight = 1.0f;

public:
    SpriteRenderer() = delete;
    explicit SpriteRenderer(GameObject* owner);
    SpriteRenderer(
        GameObject* owner,
        const wchar_t* textureFile);

    virtual ~SpriteRenderer() {}
    void OnStart() override;
    void Draw() override;
    Material* GetMaterial(uint32_t slot = 0) override;
    void SetMaterial(uint32_t slot, Material* material) override;

    void SetUVRect(float offsetX, float offsetY, float width, float height)
    {
        m_uvOffsetX = offsetX;
        m_uvOffsetY = offsetY;
        m_uvWidth = width;
        m_uvHeight = height;
    }

    void SetUVOffset(float offsetX, float offsetY)
    {
        m_uvOffsetX = offsetX;
        m_uvOffsetY = offsetY;
    }

    void SetUVSize(float width, float height)
    {
        m_uvWidth = width;
        m_uvHeight = height;
    }

    int GetTextureWidth() const { return m_textureWidth; }
    int GetTextureHeight() const { return m_textureHeight; }
    void SetColor(const Color& color) { m_baseColor = color; }

protected:
    void RegisterFields(FieldRegistry& fields) override;
};
