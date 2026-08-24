#include "pch.h"

#include "Component/ComponentInc.h"


#include "Component/SpriteRenderer.h"

#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include "System/renderer.h"

namespace
{
    template <typename VertexType>
    Microsoft::WRL::ComPtr<ID3D11Buffer> CreateSpriteVertexBuffer(const std::vector<VertexType>& vertices)
    {
        D3D11_BUFFER_DESC description{};
        description.ByteWidth = static_cast<UINT>(sizeof(VertexType) * vertices.size());
        description.Usage = D3D11_USAGE_IMMUTABLE;
        description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = vertices.data();

        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, &initialData, buffer.GetAddressOf())))
            MessageBoxA(nullptr, "スプライト頂点バッファの作成に失敗しました。", "Sprite Renderer Error", MB_OK | MB_ICONERROR);

        return buffer;
    }
}

SpriteRenderer::SpriteRenderer(
    GameObject* owner,
    const wchar_t* textureFile)
    : RendererBase(owner)
{
    m_materialInstance = MaterialManager::Load("Assets/Materials/UnlitTransparent.material");

    m_texture = textureFile ? TextureManager::LoadTexture(std::wstring(textureFile)) : TextureHandle{};
    UINT width{};
    UINT height{};
    if (TextureManager::GetTextureSize(m_texture, width, height))
    {
        m_textureWidth = static_cast<int>(width);
        m_textureHeight = static_cast<int>(height);
    }
    else
    {
        // 無効なTextureHandleは白のフォールバックへ解決し、矩形サイズは1x1とする。
        m_textureWidth = 1;
        m_textureHeight = 1;
    }

    const std::vector<PositionVertex> positions
    {
        { Vector3(-0.5f, -0.5f, 0.0f) }, { Vector3(0.5f, -0.5f, 0.0f) },
        { Vector3(-0.5f, 0.5f, 0.0f) }, { Vector3(0.5f, 0.5f, 0.0f) },
    };
    const std::vector<PrimaryTexCoordVertex> texCoords
    {
        { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 0.0f, 0.0f, 0.0f) },
        { Vector4(0.0f, 1.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 1.0f, 0.0f, 0.0f) },
    };
    const std::vector<ColorVertex> colors(4, { Color::White() });

    m_positionBuffer = CreateSpriteVertexBuffer(positions);
    m_texCoordBuffer = CreateSpriteVertexBuffer(texCoords);
    m_colorBuffer = CreateSpriteVertexBuffer(colors);

    m_renderPhase = RenderPhase::Overlay;
}

void SpriteRenderer::Draw()
{
    if (!m_materialInstance)
        return;

    // Overlayでは一時的に2D射影を使い、他の3D Rendererへ影響を残さないよう復元する。
    const XMMATRIX originalViewMatrix = Renderer::GetViewMatrix();
    const XMMATRIX originalProjectionMatrix = Renderer::GetProjectionMatrix();
    Renderer::SetWorldViewProjection2D();
    Renderer::SetWorldMatrix(CalcWorldMatrix());

    m_materialInstance->SetColor("_BaseColor", m_baseColor);
    m_materialInstance->SetVector2("_UVScale", Vector2(m_uvWidth, m_uvHeight));
    m_materialInstance->SetVector2("_UVOffset", Vector2(m_uvOffsetX, m_uvOffsetY));
    m_materialInstance->SetTexture("_MainTex", m_texture);
    m_materialInstance->Bind(0);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    bool hasAllRequiredStreams = true;
    for (const InputSlotBinding& binding : m_materialInstance->GetShaderPass(0).GetInputSlotBindings())
    {
        ID3D11Buffer* vertexBuffer = nullptr;
        switch (binding.streamType)
        {
        case VertexStreamType::Position:
            vertexBuffer = m_positionBuffer.Get();
            break;

        case VertexStreamType::PrimaryTexCoord:
            vertexBuffer = m_texCoordBuffer.Get();
            break;

        case VertexStreamType::Color:
            vertexBuffer = m_colorBuffer.Get();
            break;
        }

        // Spriteが持たないStreamは共通フォールバックで補い、Shaderごとの差異を吸収する。
        if (!vertexBuffer)
            vertexBuffer = Renderer::GetFallbackVertexBuffer(binding.streamType);

        const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
        const UINT offset = 0;
        if (!vertexBuffer || stride == 0)
        {
            hasAllRequiredStreams = false;
            break;
        }

        context->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
    }

    if (hasAllRequiredStreams)
    {
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        context->Draw(4, 0);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    Renderer::SetViewMatrix(originalViewMatrix);
    Renderer::SetProjectionMatrix(originalProjectionMatrix);
}

Material* SpriteRenderer::GetMaterial(uint32_t slot)
{
    // Spriteは単一Materialのみを持つ。
    return slot == 0 ? m_materialInstance : nullptr;
}

void SpriteRenderer::SetMaterial(uint32_t slot, Material* material)
{
    if (slot == 0)
    {
        m_materialInstance = material;
    }
}
