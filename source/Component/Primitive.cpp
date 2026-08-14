#include "pch.h"

#include "Component/Primitive.h"

#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include "System/renderer.h"

namespace
{
    template <typename VertexType>
    Microsoft::WRL::ComPtr<ID3D11Buffer> CreateVertexBuffer(const std::vector<VertexType>& vertices)
    {
        if (vertices.empty())
            return nullptr;

        D3D11_BUFFER_DESC description{};
        description.ByteWidth = static_cast<UINT>(sizeof(VertexType) * vertices.size());
        description.Usage = D3D11_USAGE_IMMUTABLE;
        description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = vertices.data();

        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, &initialData, buffer.GetAddressOf())))
            MessageBoxA(nullptr, "頂点バッファの作成に失敗しました。", "Primitive Renderer Error", MB_OK | MB_ICONERROR);

        return buffer;
    }
}

void PrimitiveRenderer::CreateMaterial(const char* shaderFilePath)
{
    GraphicsShader* shader = ShaderManager::LoadShaderFromFile(shaderFilePath);
    if (!shader)
        return;

    // Geometry Rendererは描画直前に必要な値を設定するため、Shader単位のDefaultMaterialを共有できる。
    m_materialInstance = MaterialManager::GetDefault(*shader);
}

void PrimitiveRenderer::LoadTexture(const wchar_t* textureFile)
{
    m_texture = textureFile ? TextureManager::LoadTexture(std::wstring(textureFile)) : TextureHandle{};

    UINT width{};
    UINT height{};
    if (!TextureManager::GetTextureSize(m_texture, width, height))
    {
        // 無効なTextureHandleはMaterial Bind時にマゼンタのフォールバックへ解決される。
        m_textureWidth = 1;
        m_textureHeight = 1;
        return;
    }

    m_textureWidth = static_cast<int>(width);
    m_textureHeight = static_cast<int>(height);
}

void PrimitiveRenderer::CreateMesh(
    const std::vector<PositionVertex>& positions,
    const std::vector<NormalTangentVertex>& normalTangents,
    const std::vector<PrimaryTexCoordVertex>& texCoords,
    const std::vector<ColorVertex>& colors,
    const unsigned int* indexData,
    size_t indexCount)
{
    m_mesh.Clear();
    m_mesh.SetVertexBuffer(VertexStreamType::Position, CreateVertexBuffer(positions));
    m_mesh.SetVertexBuffer(VertexStreamType::NormalTangent, CreateVertexBuffer(normalTangents));
    m_mesh.SetVertexBuffer(VertexStreamType::PrimaryTexCoord, CreateVertexBuffer(texCoords));
    m_mesh.SetVertexBuffer(VertexStreamType::Color, CreateVertexBuffer(colors));

    D3D11_BUFFER_DESC description{};
    description.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * indexCount);
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = indexData;

    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, &initialData, indexBuffer.GetAddressOf())))
    {
        MessageBoxA(nullptr, "インデックスバッファの作成に失敗しました。", "Primitive Renderer Error", MB_OK | MB_ICONERROR);
        return;
    }

    m_mesh.SetIndexBuffer(std::move(indexBuffer));
    m_mesh.indexCount = static_cast<unsigned int>(indexCount);
}

void PrimitiveRenderer::BindMesh(const XMMATRIX& worldMatrix, const Color& color) const
{
    if (!m_materialInstance)
        return;

    Renderer::SetWorldMatrix(worldMatrix);

    // 旧RendererBaseの色・UV APIを新しいMaterial Propertyへ対応付ける。
    m_materialInstance->SetColor("_BaseColor", color);
    m_materialInstance->SetVector2("_UVScale", Vector2(m_uvWidth, m_uvHeight));
    m_materialInstance->SetVector2("_UVOffset", Vector2(m_uvOffsetX, m_uvOffsetY));
    m_materialInstance->SetTexture("_MainTex", m_texture);
    m_materialInstance->Bind(0);

    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    for (const InputSlotBinding& binding : m_materialInstance->GetShaderPass(0).GetInputSlotBindings())
    {
        ID3D11Buffer* vertexBuffer = m_mesh.GetVertexBuffer(binding.streamType);
        if (!vertexBuffer)
            vertexBuffer = Renderer::GetFallbackVertexBuffer(binding.streamType);

        const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
        const UINT offset = 0;
        if (!vertexBuffer || stride == 0)
            return;

        context->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
    }

    context->IASetIndexBuffer(m_mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
}

void PrimitiveRenderer::Draw()
{
    if (!m_materialInstance || !m_mesh.GetIndexBuffer())
        return;

    BindMesh(CalcWorldMatrix(), Color(
        m_material.Diffuse.x,
        m_material.Diffuse.y,
        m_material.Diffuse.z,
        m_material.Diffuse.w));

    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Renderer::GetDeviceContext()->DrawIndexed(m_mesh.indexCount, 0, 0);
}

Quad::Quad(
    GameObject* owner,
    const wchar_t* textureFile,
    const char* /*vertexShaderFile*/,
    const char* /*pixelShaderFile*/)
    : PrimitiveRenderer(owner)
{
    CreateMaterial("shader/UnlitTransparent.shader");
    LoadTexture(textureFile);

    const float aspectRatio = static_cast<float>(m_textureWidth) / static_cast<float>(m_textureHeight);
    const float halfWidth = 0.5f;
    const float halfHeight = 0.5f / aspectRatio;

    const std::vector<PositionVertex> positions
    {
        { Vector3(-halfWidth, halfHeight, 0.0f) }, { Vector3(halfWidth, halfHeight, 0.0f) },
        { Vector3(-halfWidth, -halfHeight, 0.0f) }, { Vector3(halfWidth, -halfHeight, 0.0f) },
    };
    const std::vector<NormalTangentVertex> normalTangents(4, { Vector3(0.0f, 0.0f, -1.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) });
    const std::vector<PrimaryTexCoordVertex> texCoords
    {
        { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 0.0f, 0.0f, 0.0f) },
        { Vector4(0.0f, 1.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 1.0f, 0.0f, 0.0f) },
    };
    const std::vector<ColorVertex> colors(4, { Color::White() });

    CreateMesh(positions, normalTangents, texCoords, colors, QUAD_INDEX_BUFFER, QUAD_INDEX_COUNT);
}

Plane::Plane(
    GameObject* owner,
    const wchar_t* textureFile,
    const char* /*vertexShaderFile*/,
    const char* /*pixelShaderFile*/)
    : PrimitiveRenderer(owner)
{
    CreateMaterial("shader/UnlitTexture.shader");
    LoadTexture(textureFile);

    constexpr float textureRepeat = 30.0f;
    const float aspectRatio = static_cast<float>(m_textureWidth) / static_cast<float>(m_textureHeight);
    const float halfWidth = 5.0f;
    const float halfHeight = 5.0f / aspectRatio;

    const std::vector<PositionVertex> positions
    {
        { Vector3(-halfWidth, 0.0f, halfHeight) }, { Vector3(halfWidth, 0.0f, halfHeight) },
        { Vector3(-halfWidth, 0.0f, -halfHeight) }, { Vector3(halfWidth, 0.0f, -halfHeight) },
    };
    const std::vector<NormalTangentVertex> normalTangents(4, { Vector3(0.0f, 1.0f, 0.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) });
    const std::vector<PrimaryTexCoordVertex> texCoords
    {
        { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(textureRepeat, 0.0f, 0.0f, 0.0f) },
        { Vector4(0.0f, textureRepeat, 0.0f, 0.0f) }, { Vector4(textureRepeat, textureRepeat, 0.0f, 0.0f) },
    };
    const std::vector<ColorVertex> colors(4, { Color::White() });

    CreateMesh(positions, normalTangents, texCoords, colors, PLANE_INDEX_BUFFER, PLANE_INDEX_COUNT);
}

Cube::Cube(
    GameObject* owner,
    const wchar_t* textureFile,
    const char* /*vertexShaderFile*/,
    const char* /*pixelShaderFile*/)
    : PrimitiveRenderer(owner)
{
    CreateMaterial("shader/UnlitTexture.shader");
    LoadTexture(textureFile);

    const Vector3 cubePositions[]
    {
        { -0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f },
        { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f },
        { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f }, { -0.5f, -0.5f, 0.5f },
        { -0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
        { 0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f }, { 0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, 0.5f },
        { -0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, -0.5f }, { -0.5f, -0.5f, 0.5f }, { -0.5f, -0.5f, -0.5f },
    };
    const Vector3 faceNormals[]
    {
        { 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
    };
    const Vector4 faceTexCoords[]
    {
        { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f, 0.0f },
    };

    std::vector<PositionVertex> positions;
    std::vector<NormalTangentVertex> normalTangents;
    std::vector<PrimaryTexCoordVertex> texCoords;
    std::vector<ColorVertex> colors;
    positions.reserve(24);
    normalTangents.reserve(24);
    texCoords.reserve(24);
    colors.reserve(24);

    for (size_t index = 0; index < 24; ++index)
    {
        positions.push_back({ cubePositions[index] });
        normalTangents.push_back({ faceNormals[index / 4], Vector4(1.0f, 0.0f, 0.0f, 1.0f) });
        texCoords.push_back({ faceTexCoords[index % 4] });
        colors.push_back({ Color::White() });
    }

    CreateMesh(positions, normalTangents, texCoords, colors, CUBE_INDEX_BUFFER, CUBE_INDEX_COUNT);
}
