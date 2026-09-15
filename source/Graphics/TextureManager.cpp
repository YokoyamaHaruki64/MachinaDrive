#include "pch.h"
#include "Graphics/TextureManager.h"
#include "System/Diagnostics/Logger.h"
#include "System/Rendering/renderer.h"

std::unordered_map<TextureID, TextureManager::TextureResource> TextureManager::m_textures;
std::unordered_map<std::wstring, TextureID> TextureManager::m_textureIDsByPath;
std::unordered_map<std::string, TextureID> TextureManager::m_textureIDsByName;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureManager::m_fallbackTexture;
TextureID TextureManager::m_nextTextureID{};

bool TextureManager::CreateFallbackTexture()
{
    // Texture未設定時は見た目への影響を抑える白を使い、マゼンタはMaterial用の将来予約とする。
    const uint32_t whitePixel = 0xFFFFFFFF;
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
    initialData.pSysMem = &whitePixel;
    initialData.SysMemPitch = sizeof(whitePixel);

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
    m_textureIDsByName.clear();
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
    resource.sourcePath = std::filesystem::path(normalizedPath).generic_string();

    const TextureID textureID = m_nextTextureID++;
    m_textures.emplace(textureID, std::move(resource));
    m_textureIDsByPath.emplace(normalizedPath, textureID);
    return TextureHandle(textureID);
}

TextureHandle TextureManager::LoadTexture(const std::string& filePath)
{
    return LoadTexture(std::filesystem::path(filePath).wstring());
}

TextureHandle TextureManager::LoadTextureFromMemory(const void* data, size_t dataSize)
{
    if (!data || dataSize == 0 || !Init())
        return TextureHandle{};

    DirectX::TexMetadata metadata{};
    DirectX::ScratchImage image{};
    HRESULT result = DirectX::LoadFromWICMemory(data, dataSize, DirectX::WIC_FLAGS_NONE, &metadata, image);
    if (FAILED(result))
        result = DirectX::LoadFromDDSMemory(data, dataSize, DirectX::DDS_FLAGS_NONE, &metadata, image);

    if (FAILED(result))
        return TextureHandle{};

    TextureResource resource{};
    if (FAILED(DirectX::CreateShaderResourceView(Renderer::GetDevice(), image.GetImages(), image.GetImageCount(), metadata, resource.shaderResourceView.GetAddressOf())))
        return TextureHandle{};

    resource.width = static_cast<UINT>(metadata.width);
    resource.height = static_cast<UINT>(metadata.height);

    const TextureID textureID = m_nextTextureID++;
    m_textures.emplace(textureID, std::move(resource));
    return TextureHandle(textureID);
}

TextureHandle TextureManager::CreateTextureFromBGRA8(const void* pixels, UINT width, UINT height)
{
    if (!pixels || width == 0 || height == 0 || !Init())
        return TextureHandle{};

    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = pixels;
    initialData.SysMemPitch = width * sizeof(uint32_t);

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    if (FAILED(Renderer::GetDevice()->CreateTexture2D(&textureDesc, &initialData, texture.GetAddressOf())))
        return TextureHandle{};

    TextureResource resource{};
    if (FAILED(Renderer::GetDevice()->CreateShaderResourceView(texture.Get(), nullptr, resource.shaderResourceView.GetAddressOf())))
        return TextureHandle{};

    resource.width = width;
    resource.height = height;

    const TextureID textureID = m_nextTextureID++;
    m_textures.emplace(textureID, std::move(resource));
    return TextureHandle(textureID);
}

TextureHandle TextureManager::CreateRenderTexture(const std::string& name)
{
    const UINT width = Renderer::GetGameWidth() > 0 ? Renderer::GetGameWidth() : SCREEN_WIDTH;
    const UINT height = Renderer::GetGameHeight() > 0 ? Renderer::GetGameHeight() : SCREEN_HEIGHT;
    return CreateRenderTexture(name, width, height);
}

TextureHandle TextureManager::CreateRenderTexture(const std::string& name, UINT width, UINT height)
{
    if (name.empty() || width == 0 || height == 0 || !Init())
        return TextureHandle{};

    // 同じ名前のRTは同一Handleを返し、フレーム中に重複したGPUリソースを作らない。
    const auto existing = m_textureIDsByName.find(name);
    if (existing != m_textureIDsByName.end())
    {
        TextureResource& resource = m_textures.at(existing->second);
        if (resource.width == width && resource.height == height)
            return TextureHandle(existing->second);

        // Handleを保持したままGPUリソースだけ置き換え、Material側の参照を無効化しない。
        D3D11_TEXTURE2D_DESC description{};
        description.Width = width;
        description.Height = height;
        description.MipLevels = 1;
        description.ArraySize = 1;
        description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        description.SampleDesc.Count = 1;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        TextureResource replacement{};
        if (FAILED(Renderer::GetDevice()->CreateTexture2D(&description, nullptr, replacement.texture.GetAddressOf())) ||
            FAILED(Renderer::GetDevice()->CreateRenderTargetView(
                replacement.texture.Get(), nullptr, replacement.renderTargetView.GetAddressOf())) ||
            FAILED(Renderer::GetDevice()->CreateShaderResourceView(
                replacement.texture.Get(), nullptr, replacement.shaderResourceView.GetAddressOf())))
        {
            Logger::Error("RenderTextureの再生成に失敗しました。");
            return TextureHandle{};
        }

        replacement.width = width;
        replacement.height = height;
        resource = std::move(replacement);
        return TextureHandle(existing->second);
    }

    D3D11_TEXTURE2D_DESC description{};
    description.Width = width;
    description.Height = height;
    description.MipLevels = 1;
    description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_DEFAULT;
    description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    TextureResource resource{};
    if (FAILED(Renderer::GetDevice()->CreateTexture2D(&description, nullptr, resource.texture.GetAddressOf())) ||
        FAILED(Renderer::GetDevice()->CreateRenderTargetView(resource.texture.Get(), nullptr, resource.renderTargetView.GetAddressOf())) ||
        FAILED(Renderer::GetDevice()->CreateShaderResourceView(resource.texture.Get(), nullptr, resource.shaderResourceView.GetAddressOf())))
    {
        Logger::Error("RenderTextureの作成に失敗しました。");
        return TextureHandle{};
    }

    resource.width = width;
    resource.height = height;

    const TextureID textureID = m_nextTextureID++;
    m_textures.emplace(textureID, std::move(resource));
    m_textureIDsByName.emplace(name, textureID);
    return TextureHandle(textureID);
}

ID3D11ShaderResourceView* TextureManager::GetShaderResourceView(TextureHandle texture)
{
    const auto resource = m_textures.find(texture.id);
    return resource == m_textures.end() ? m_fallbackTexture.Get() : resource->second.shaderResourceView.Get();
}

ID3D11RenderTargetView* TextureManager::GetRenderTargetView(TextureHandle texture)
{
    const auto resource = m_textures.find(texture.id);
    return resource == m_textures.end() ? nullptr : resource->second.renderTargetView.Get();
}

ID3D11Texture2D* TextureManager::GetTexture(TextureHandle texture)
{
    const auto resource = m_textures.find(texture.id);
    return resource == m_textures.end() ? nullptr : resource->second.texture.Get();
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

std::optional<std::string> TextureManager::GetSourcePath(TextureHandle texture)
{
    const auto resource = m_textures.find(texture.id);
    if (resource == m_textures.end() || resource->second.sourcePath.empty())
    {
        return std::nullopt;
    }
    return resource->second.sourcePath;
}
