#include "pch.h"
#include "Graphics/TextureManager.h"
#include "System/renderer.h"

std::unordered_map<TextureID, TextureManager::TextureResource> TextureManager::m_textures;
std::unordered_map<std::wstring, TextureID> TextureManager::m_textureIDsByPath;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureManager::m_fallbackTexture;
TextureID TextureManager::m_nextTextureID{};

bool TextureManager::CreateFallbackTexture()
{
    const uint32_t magentaPixel = 0xFFFF00FF;
    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = 1;
    textureDesc.Height = 1;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = &magentaPixel;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    if (FAILED(Renderer::GetDevice()->CreateTexture2D(&textureDesc, &initialData, texture.GetAddressOf())))
        return false;

    return SUCCEEDED(Renderer::GetDevice()->CreateShaderResourceView(texture.Get(), nullptr, m_fallbackTexture.GetAddressOf()));
}

std::wstring TextureManager::NormalizePath(const std::wstring& filePath)
{
    return std::filesystem::path(filePath).lexically_normal().wstring();
}

bool TextureManager::Init()
{
    return m_fallbackTexture || CreateFallbackTexture();
}

void TextureManager::Uninit()
{
    m_textures.clear();
    m_textureIDsByPath.clear();
    m_fallbackTexture.Reset();
    m_nextTextureID = 0;
}

TextureHandle TextureManager::LoadTexture(const std::wstring& filePath)
{
    if (filePath.empty() || !Init())
        return TextureHandle{};

    const std::wstring normalizedPath = NormalizePath(filePath);
    const auto existing = m_textureIDsByPath.find(normalizedPath);
    if (existing != m_textureIDsByPath.end())
        return TextureHandle(existing->second);

    DirectX::TexMetadata metadata{};
    DirectX::ScratchImage image{};
    if (FAILED(DirectX::LoadFromWICFile(normalizedPath.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, image)))
        return TextureHandle{};

    TextureResource resource{};
    if (FAILED(DirectX::CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(), image.GetImageCount(), metadata, resource.shaderResourceView.GetAddressOf())))
        return TextureHandle{};

    resource.width = static_cast<UINT>(metadata.width);
    resource.height = static_cast<UINT>(metadata.height);

    const TextureID textureID = m_nextTextureID++;
    m_textures.emplace(textureID, std::move(resource));
    m_textureIDsByPath.emplace(normalizedPath, textureID);
    return TextureHandle(textureID);
}

TextureHandle TextureManager::LoadTexture(const std::string& filePath)
{
    return LoadTexture(std::filesystem::path(filePath).wstring());
}

ID3D11ShaderResourceView* TextureManager::GetShaderResourceView(TextureHandle texture)
{
    const auto resource = m_textures.find(texture.id);
    return resource == m_textures.end() ? m_fallbackTexture.Get() : resource->second.shaderResourceView.Get();
}

bool TextureManager::GetTextureSize(TextureHandle texture, UINT& outWidth, UINT& outHeight)
{
    const auto resource = m_textures.find(texture.id);
    if (resource == m_textures.end())
        return false;

    outWidth = resource->second.width;
    outHeight = resource->second.height;
    return true;
}
