#pragma once
#include "System/RendererBase.h"

#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

class SpriteRenderer : public RendererBase
{
private:
    ComPtr<ID3D11Buffer> m_vertexBuffer;

    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;

    ComPtr<ID3D11ShaderResourceView> m_texture;
    int m_textureWidth = 0;
    int m_textureHeight = 0;

    float m_uvOffsetX = 0.0f;
    float m_uvOffsetY = 0.0f;
    float m_uvWidth = 1.0f;
    float m_uvHeight = 1.0f;

    static const char* DEFAULT_VERTEX_SHADER_FILE;
    static const char* DEFAULT_PIXEL_SHADER_FILE;
public:
    SpriteRenderer() = delete;
    SpriteRenderer(
        GameObject* owner,
        const wchar_t* textureFile,
        const char* vertexShaderFile = DEFAULT_VERTEX_SHADER_FILE,
        const char* pixelShaderFile = DEFAULT_PIXEL_SHADER_FILE
    );

    virtual ~SpriteRenderer() {}
    void Draw() override;

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
};
