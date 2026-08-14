#pragma once

#include "Graphics/GraphicInc.h"
#include <unordered_map>
#include <wrl/client.h>

class TextureManager
{
    struct TextureResource
    {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
        UINT width{};
        UINT height{};
    };

    static std::unordered_map<TextureID, TextureResource> m_textures;
    static std::unordered_map<std::wstring, TextureID> m_textureIDsByPath;
    static std::unordered_map<std::string, TextureID> m_textureIDsByName;
    static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fallbackTexture;
    static TextureID m_nextTextureID;

    static bool CreateFallbackTexture();
    static std::wstring NormalizePath(const std::wstring& filePath);

public:
    static bool Init();
    static void Uninit();

    static TextureHandle LoadTexture(const std::wstring& filePath);
    static TextureHandle LoadTexture(const std::string& filePath);
    static TextureHandle LoadTextureFromMemory(const void* data, size_t dataSize);
    static TextureHandle CreateTextureFromBGRA8(const void* pixels, UINT width, UINT height);
    static TextureHandle CreateRenderTexture(const std::string& name);
    static TextureHandle CreateRenderTexture(const std::string& name, UINT width, UINT height);
    static ID3D11ShaderResourceView* GetShaderResourceView(TextureHandle texture);
    static ID3D11RenderTargetView* GetRenderTargetView(TextureHandle texture);
    static ID3D11Texture2D* GetTexture(TextureHandle texture);
    static bool GetTextureSize(TextureHandle texture, UINT& outWidth, UINT& outHeight);
};
