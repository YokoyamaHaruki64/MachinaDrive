#include "pch.h"

#include "Graphics/ModelManager.h"

#include "Graphics/MaterialManager.h"
#include "Graphics/ModelLoader.h"
#include "Graphics/Shader/ShaderManager.h"
#include "System/Rendering/renderer.h"

#include <algorithm>
#include <memory>
#include <unordered_map>

namespace
{
    std::unordered_map<std::string, std::unique_ptr<ModelAsset>> g_modelAssets;
    uint32_t g_nextRuntimeMeshID = 0x80000000;

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
            MessageBoxA(nullptr, "Primitive Meshの頂点バッファ作成に失敗しました。", "Model Manager Error", MB_OK | MB_ICONERROR);

        return buffer;
    }

    ModelAsset* RegisterPrimitive(
        const char* name,
        const std::vector<PositionVertex>& positions,
        const std::vector<NormalTangentVertex>& normalTangents,
        const std::vector<PrimaryTexCoordVertex>& texCoords,
        const std::vector<ColorVertex>& colors,
        const std::vector<unsigned int>& indices)
    {
        auto modelAsset = std::make_unique<ModelAsset>();

        // 組み込みPrimitiveは共通のDefaultLit Materialを既定値とし、必要なときだけRenderer側でoverrideする。
        GraphicsShader* defaultShader = ShaderManager::GetDefaultShader();
        Material* defaultMaterial = defaultShader ? MaterialManager::GetDefault(*defaultShader) : nullptr;
        if (!defaultMaterial)
        {
            MessageBoxA(nullptr, "Primitive Meshの既定Materialを取得できません。", "Model Manager Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }

        modelAsset->embeddedMaterials.push_back(defaultMaterial);

        Mesh mesh{};
        mesh.meshID = g_nextRuntimeMeshID++;
        mesh.materialSlot = 0;
        mesh.indexCount = static_cast<unsigned int>(indices.size());
        mesh.SetVertexBuffer(VertexStreamType::Position, CreateVertexBuffer(positions));
        mesh.SetVertexBuffer(VertexStreamType::NormalTangent, CreateVertexBuffer(normalTangents));
        mesh.SetVertexBuffer(VertexStreamType::PrimaryTexCoord, CreateVertexBuffer(texCoords));
        mesh.SetVertexBuffer(VertexStreamType::Color, CreateVertexBuffer(colors));

        D3D11_BUFFER_DESC description{};
        description.ByteWidth = static_cast<UINT>(sizeof(unsigned int) * indices.size());
        description.Usage = D3D11_USAGE_IMMUTABLE;
        description.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = indices.data();
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, &initialData, indexBuffer.GetAddressOf())))
        {
            MessageBoxA(nullptr, "Primitive Meshのインデックスバッファ作成に失敗しました。", "Model Manager Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }

        mesh.SetIndexBuffer(std::move(indexBuffer));
        modelAsset->meshes.push_back(std::move(mesh));
        modelAsset->rootNode.meshIndices.push_back(0);

        ModelAsset* result = modelAsset.get();
        g_modelAssets.emplace(name, std::move(modelAsset));
        return result;
    }

    void CreatePrimitives()
    {
        const std::vector<unsigned int> quadIndices{ 0, 1, 2, 2, 1, 3 };
        const std::vector<PositionVertex> quadPositions
        {
            { Vector3(-0.5f, 0.5f, 0.0f) }, { Vector3(0.5f, 0.5f, 0.0f) },
            { Vector3(-0.5f, -0.5f, 0.0f) }, { Vector3(0.5f, -0.5f, 0.0f) },
        };
        const std::vector<NormalTangentVertex> quadNormalTangents(4,
            { Vector3(0.0f, 0.0f, -1.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) });
        const std::vector<PrimaryTexCoordVertex> quadTexCoords
        {
            { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 0.0f, 0.0f, 0.0f) },
            { Vector4(0.0f, 1.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 1.0f, 0.0f, 0.0f) },
        };
        const std::vector<ColorVertex> quadColors(4, { Color::White() });

        RegisterPrimitive("__primitive_quad", quadPositions, quadNormalTangents, quadTexCoords, quadColors, quadIndices);

        const std::vector<PositionVertex> planePositions
        {
            { Vector3(-5.0f, 0.0f, 5.0f) }, { Vector3(5.0f, 0.0f, 5.0f) },
            { Vector3(-5.0f, 0.0f, -5.0f) }, { Vector3(5.0f, 0.0f, -5.0f) },
        };
        const std::vector<NormalTangentVertex> planeNormalTangents(4,
            { Vector3(0.0f, 1.0f, 0.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) });
        const std::vector<PrimaryTexCoordVertex> planeTexCoords
        {
            { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(30.0f, 0.0f, 0.0f, 0.0f) },
            { Vector4(0.0f, 30.0f, 0.0f, 0.0f) }, { Vector4(30.0f, 30.0f, 0.0f, 0.0f) },
        };

        RegisterPrimitive("__primitive_plane", planePositions, planeNormalTangents, planeTexCoords, quadColors, quadIndices);

        const std::vector<Vector3> cubePositions
        {
            { -0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f },
            { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f }, { -0.5f, -0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f },
            { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f }, { 0.5f, -0.5f, 0.5f }, { -0.5f, -0.5f, 0.5f },
            { -0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, -0.5f }, { -0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, -0.5f },
            { 0.5f, 0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f }, { 0.5f, -0.5f, -0.5f }, { 0.5f, -0.5f, 0.5f },
            { -0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, -0.5f }, { -0.5f, -0.5f, 0.5f }, { -0.5f, -0.5f, -0.5f },
        };
        const std::vector<Vector3> faceNormals
        {
            { 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },
            { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
        };

        std::vector<PositionVertex> cubePositionVertices;
        std::vector<NormalTangentVertex> cubeNormalTangents;
        std::vector<PrimaryTexCoordVertex> cubeTexCoords;
        std::vector<ColorVertex> cubeColors;
        cubePositionVertices.reserve(24);
        cubeNormalTangents.reserve(24);
        cubeTexCoords.reserve(24);
        cubeColors.reserve(24);

        for (size_t index = 0; index < cubePositions.size(); ++index)
        {
            cubePositionVertices.push_back({ cubePositions[index] });
            cubeNormalTangents.push_back({ faceNormals[index / 4], Vector4(1.0f, 0.0f, 0.0f, 1.0f) });
            cubeTexCoords.push_back(quadTexCoords[index % 4]);
            cubeColors.push_back({ Color::White() });
        }

        const std::vector<unsigned int> cubeIndices
        {
            0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6,
            8, 9, 10, 9, 11, 10, 12, 13, 14, 13, 15, 14,
            16, 17, 18, 17, 19, 18, 20, 21, 22, 21, 23, 22,
        };

        RegisterPrimitive("__primitive_cube", cubePositionVertices, cubeNormalTangents, cubeTexCoords, cubeColors, cubeIndices);
    }
}

void ModelManager::Init()
{
    CreatePrimitives();
}

void ModelManager::Uninit()
{
    for (auto& [filePath, modelAsset] : g_modelAssets)
        ModelLoader::Release(*modelAsset);

    g_modelAssets.clear();
}

ModelAsset* ModelManager::Load(const std::string& filePath)
{
    if (const auto found = g_modelAssets.find(filePath); found != g_modelAssets.end())
        return found->second.get();

    auto modelAsset = std::make_unique<ModelAsset>();
    if (!ModelLoader::Load(filePath, *modelAsset))
        return nullptr;

    ModelAsset* result = modelAsset.get();
    g_modelAssets.emplace(filePath, std::move(modelAsset));
    return result;
}

ModelAsset* ModelManager::CreateRuntimeModel(
    const std::string& assetName,
    const RuntimeMeshData& meshData,
    Material* material)
{
    if (assetName.empty() || meshData.positions.empty() || meshData.indices.empty())
    {
        MessageBoxA(nullptr, "Runtime Meshにはアセット名、Position頂点、Indexが必要です。", "Model Manager Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    if (const auto found = g_modelAssets.find(assetName); found != g_modelAssets.end())
    {
        return found->second.get();
    }

    const size_t vertexCount = meshData.positions.size();
    const bool validVertexStreams =
        (meshData.normalTangents.empty() || meshData.normalTangents.size() == vertexCount) &&
        (meshData.primaryTexCoords.empty() || meshData.primaryTexCoords.size() == vertexCount) &&
        (meshData.secondaryTexCoords.empty() || meshData.secondaryTexCoords.size() == vertexCount) &&
        (meshData.extraTexCoords.empty() || meshData.extraTexCoords.size() == vertexCount) &&
        (meshData.colors.empty() || meshData.colors.size() == vertexCount);
    const bool validIndices = std::all_of(meshData.indices.begin(), meshData.indices.end(),
        [vertexCount](uint32_t index) { return index < vertexCount; });
    if (!validVertexStreams || !validIndices)
    {
        MessageBoxA(nullptr, "Runtime Meshの頂点ストリームまたはIndexが不正です。", "Model Manager Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    if (!material)
    {
        GraphicsShader* defaultShader = ShaderManager::GetDefaultShader();
        material = defaultShader ? MaterialManager::GetDefault(*defaultShader) : nullptr;
    }
    if (!material)
    {
        MessageBoxA(nullptr, "Runtime MeshのMaterialを取得できません。", "Model Manager Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    Mesh mesh{};
    mesh.meshID = g_nextRuntimeMeshID++;
    mesh.materialSlot = 0;
    mesh.indexCount = static_cast<unsigned int>(meshData.indices.size());

    const auto setVertexBuffer = [&mesh](VertexStreamType streamType, const auto& vertices)
    {
        if (vertices.empty())
            return true;

        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer = CreateVertexBuffer(vertices);
        if (!buffer)
            return false;

        mesh.SetVertexBuffer(streamType, std::move(buffer));
        return true;
    };
    if (!setVertexBuffer(VertexStreamType::Position, meshData.positions) ||
        !setVertexBuffer(VertexStreamType::NormalTangent, meshData.normalTangents) ||
        !setVertexBuffer(VertexStreamType::PrimaryTexCoord, meshData.primaryTexCoords) ||
        !setVertexBuffer(VertexStreamType::SecondaryTexCoord, meshData.secondaryTexCoords) ||
        !setVertexBuffer(VertexStreamType::ExtraTexCoord, meshData.extraTexCoords) ||
        !setVertexBuffer(VertexStreamType::Color, meshData.colors))
    {
        return nullptr;
    }

    D3D11_BUFFER_DESC description{};
    description.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * meshData.indices.size());
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = meshData.indices.data();
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, &initialData, indexBuffer.GetAddressOf())))
    {
        MessageBoxA(nullptr, "Runtime Meshのインデックスバッファ作成に失敗しました。", "Model Manager Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    mesh.SetIndexBuffer(std::move(indexBuffer));
    auto modelAsset = std::make_unique<ModelAsset>();
    modelAsset->embeddedMaterials.push_back(material);
    modelAsset->meshes.push_back(std::move(mesh));
    modelAsset->rootNode.meshIndices.push_back(0);

    ModelAsset* result = modelAsset.get();
    g_modelAssets.emplace(assetName, std::move(modelAsset));
    return result;
}

ModelAsset* ModelManager::GetPrimitive(PrimitiveType primitiveType)
{
    const char* name = nullptr;
    switch (primitiveType)
    {
    case PrimitiveType::Quad:
        name = "__primitive_quad";
        break;

    case PrimitiveType::Plane:
        name = "__primitive_plane";
        break;

    case PrimitiveType::Cube:
        name = "__primitive_cube";
        break;
    }

    const auto found = name ? g_modelAssets.find(name) : g_modelAssets.end();
    return found == g_modelAssets.end() ? nullptr : found->second.get();
}

std::optional<std::string> ModelManager::GetSourcePath(const ModelAsset* modelAsset)
{
    for (const auto& [path, asset] : g_modelAssets)
    {
        if (asset.get() == modelAsset)
        {
            return path;
        }
    }
    return std::nullopt;
}
