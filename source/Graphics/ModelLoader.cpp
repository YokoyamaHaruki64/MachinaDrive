#include "pch.h"

#include "Graphics/ModelLoader.h"

#include "Graphics/Material.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/TextureManager.h"
#include "System/Diagnostics/Logger.h"
#include "System/Rendering/renderer.h"

#include <Assimp/Importer.hpp>
#include <Assimp/material.h>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>
#include <Assimp/texture.h>

#include <algorithm>
#include <cctype>
#include <filesystem>

// 導入済みのAssimp SDKは、構成共通の静的ライブラリとして配布されている。
#pragma comment(lib, "External/Library/assimp/assimp-vc145-mt.lib")

namespace
{
    uint32_t g_nextMeshID = 1;

    bool IsSupportedModelFile(const std::string& filePath)
    {
        std::string extension = std::filesystem::path(filePath).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char character) { return static_cast<char>(std::tolower(character)); });

        return extension == ".obj" || extension == ".fbx" || extension == ".gltf";
    }

    void ShowLoadError(const std::string& message)
    {
        Logger::Error(message);
    }

    TextureHandle LoadMaterialTexture(
        const aiScene& scene,
        const aiString& texturePath,
        const std::filesystem::path& modelDirectory)
    {
        // Assimpは*0形式とファイル名形式の両方から埋め込みTextureを検索できる。
        if (const aiTexture* embeddedTexture = scene.GetEmbeddedTexture(texturePath.C_Str()))
        {
            if (embeddedTexture->mHeight == 0)
            {
                return TextureManager::LoadTextureFromMemory(embeddedTexture->pcData, embeddedTexture->mWidth);
            }

            return TextureManager::CreateTextureFromBGRA8(
                embeddedTexture->pcData,
                embeddedTexture->mWidth,
                embeddedTexture->mHeight);
        }

        const std::string relativeTexturePath = texturePath.C_Str();
        if (relativeTexturePath.empty())
            return TextureHandle{};

        return TextureManager::LoadTexture((modelDirectory / std::filesystem::path(relativeTexturePath)).string());
    }

    Material* CreateMaterial(
        const aiScene& scene,
        const aiMaterial& sourceMaterial,
        const std::filesystem::path& modelDirectory,
        const std::string& materialName)
    {
        GraphicsShader* defaultShader = ShaderManager::GetDefaultShader();
        if (!defaultShader)
            return nullptr;

        Material* material = MaterialManager::Create(materialName, *defaultShader);
        if (!material)
            return nullptr;

        // Assimpの色情報をDefaultLitのPropertyへ対応付ける。
        aiColor4D color{};
        if (aiGetMaterialColor(&sourceMaterial, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
            material->SetColor("_BaseColor", Color(color.r, color.g, color.b, color.a));

        if (aiGetMaterialColor(&sourceMaterial, AI_MATKEY_COLOR_EMISSIVE, &color) == AI_SUCCESS)
            material->SetColor("_Emission", Color(color.r, color.g, color.b, color.a));

        float shininess{};
        if (sourceMaterial.Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            material->SetFloat("_Shininess", shininess);

        aiString texturePath;
        if (sourceMaterial.GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) != AI_SUCCESS)
            return material;

        material->SetTexture("_MainTex", LoadMaterialTexture(scene, texturePath, modelDirectory));
        return material;
    }

    bool CreateBuffer(UINT bindFlags, const void* data, UINT byteWidth, Microsoft::WRL::ComPtr<ID3D11Buffer>& outBuffer)
    {
        D3D11_BUFFER_DESC description{};
        description.ByteWidth = byteWidth;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = bindFlags;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = data;

        return SUCCEEDED(Renderer::GetDevice()->CreateBuffer(&description, &initialData, outBuffer.GetAddressOf()));
    }

    Vector4 GetTexCoord(const aiMesh& sourceMesh, unsigned int texCoordIndex, unsigned int vertexIndex)
    {
        if (!sourceMesh.HasTextureCoords(texCoordIndex))
            return Vector4(0.0f, 0.0f, 0.0f, 0.0f);

        const aiVector3D& texCoord = sourceMesh.mTextureCoords[texCoordIndex][vertexIndex];
        return Vector4(texCoord.x, texCoord.y, texCoord.z, 0.0f);
    }

    Vector4 GetTangent(const aiMesh& sourceMesh, unsigned int vertexIndex, const aiVector3D& normal)
    {
        if (!sourceMesh.HasTangentsAndBitangents())
        {
            // Tangentがない場合もNormal Streamは有効なので、Normalに直交する基底を生成する。
            const aiVector3D referenceAxis = std::abs(normal.y) < 0.999f
                ? aiVector3D(0.0f, 1.0f, 0.0f)
                : aiVector3D(1.0f, 0.0f, 0.0f);
            aiVector3D generatedTangent = normal ^ referenceAxis;
            generatedTangent.Normalize();
            return Vector4(generatedTangent.x, generatedTangent.y, generatedTangent.z, 1.0f);
        }

        const aiVector3D& tangent = sourceMesh.mTangents[vertexIndex];
        const aiVector3D& bitangent = sourceMesh.mBitangents[vertexIndex];
        const aiVector3D cross = normal ^ tangent;
        const float handedness = (cross * bitangent) < 0.0f ? -1.0f : 1.0f;

        return Vector4(tangent.x, tangent.y, tangent.z, handedness);
    }

    bool CreateMesh(const aiMesh& sourceMesh, Mesh& outMesh)
    {
        if (!sourceMesh.HasPositions() || sourceMesh.mNumFaces == 0)
            return false;

        std::vector<PositionVertex> positions;
        std::vector<NormalTangentVertex> normalTangents;
        std::vector<PrimaryTexCoordVertex> primaryTexCoords;
        std::vector<SecondaryTexCoordVertex> secondaryTexCoords;
        std::vector<ExtraTexCoordVertex> extraTexCoords;
        std::vector<ColorVertex> colors;
        std::vector<unsigned int> indices;
        positions.reserve(sourceMesh.mNumVertices);
        normalTangents.reserve(sourceMesh.mNumVertices);
        primaryTexCoords.reserve(sourceMesh.mNumVertices);
        secondaryTexCoords.reserve(sourceMesh.mNumVertices);
        extraTexCoords.reserve(sourceMesh.mNumVertices);
        colors.reserve(sourceMesh.mNumVertices);

        const bool hasNormalTangent = sourceMesh.HasNormals();
        const bool hasPrimaryTexCoord = sourceMesh.HasTextureCoords(0);
        const bool hasSecondaryTexCoord = sourceMesh.HasTextureCoords(1);
        const bool hasColor = sourceMesh.HasVertexColors(0);
        bool hasExtraTexCoords = false;
        for (unsigned int texCoordIndex = 2; texCoordIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++texCoordIndex)
            hasExtraTexCoords |= sourceMesh.HasTextureCoords(texCoordIndex);

        for (unsigned int vertexIndex = 0; vertexIndex < sourceMesh.mNumVertices; ++vertexIndex)
        {
            const aiVector3D& position = sourceMesh.mVertices[vertexIndex];
            positions.push_back({ Vector3(position.x, position.y, position.z) });

            if (hasNormalTangent)
            {
                const aiVector3D& normal = sourceMesh.mNormals[vertexIndex];
                normalTangents.push_back({
                    Vector3(normal.x, normal.y, normal.z),
                    GetTangent(sourceMesh, vertexIndex, normal),
                });
            }

            if (hasPrimaryTexCoord)
                primaryTexCoords.push_back({ GetTexCoord(sourceMesh, 0, vertexIndex) });

            if (hasSecondaryTexCoord)
            {
                secondaryTexCoords.push_back({ GetTexCoord(sourceMesh, 1, vertexIndex) });
            }

            if (hasExtraTexCoords)
            {
                ExtraTexCoordVertex extraTexCoord{};
                Vector4* destinationTexCoords[]
                {
                    &extraTexCoord.texCoord2,
                    &extraTexCoord.texCoord3,
                    &extraTexCoord.texCoord4,
                    &extraTexCoord.texCoord5,
                    &extraTexCoord.texCoord6,
                    &extraTexCoord.texCoord7,
                };

                for (unsigned int texCoordIndex = 2; texCoordIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++texCoordIndex)
                {
                    *destinationTexCoords[texCoordIndex - 2] = GetTexCoord(sourceMesh, texCoordIndex, vertexIndex);
                }

                extraTexCoords.push_back(extraTexCoord);
            }

            if (hasColor)
            {
                const aiColor4D& color = sourceMesh.mColors[0][vertexIndex];
                colors.push_back({ Color(color.r, color.g, color.b, color.a) });
            }
        }

        for (unsigned int faceIndex = 0; faceIndex < sourceMesh.mNumFaces; ++faceIndex)
        {
            const aiFace& face = sourceMesh.mFaces[faceIndex];
            if (face.mNumIndices != 3)
                continue;

            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }

        if (indices.empty())
            return false;

        Microsoft::WRL::ComPtr<ID3D11Buffer> positionBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> normalTangentBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> primaryTexCoordBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> secondaryTexCoordBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> extraTexCoordBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> colorBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
        if (!CreateBuffer(D3D11_BIND_VERTEX_BUFFER, positions.data(), static_cast<UINT>(sizeof(PositionVertex) * positions.size()), positionBuffer) ||
            (hasNormalTangent && !CreateBuffer(D3D11_BIND_VERTEX_BUFFER, normalTangents.data(), static_cast<UINT>(sizeof(NormalTangentVertex) * normalTangents.size()), normalTangentBuffer)) ||
            (hasPrimaryTexCoord && !CreateBuffer(D3D11_BIND_VERTEX_BUFFER, primaryTexCoords.data(), static_cast<UINT>(sizeof(PrimaryTexCoordVertex) * primaryTexCoords.size()), primaryTexCoordBuffer)) ||
            (hasSecondaryTexCoord && !CreateBuffer(D3D11_BIND_VERTEX_BUFFER, secondaryTexCoords.data(), static_cast<UINT>(sizeof(SecondaryTexCoordVertex) * secondaryTexCoords.size()), secondaryTexCoordBuffer)) ||
            (hasExtraTexCoords && !CreateBuffer(D3D11_BIND_VERTEX_BUFFER, extraTexCoords.data(), static_cast<UINT>(sizeof(ExtraTexCoordVertex) * extraTexCoords.size()), extraTexCoordBuffer)) ||
            (hasColor && !CreateBuffer(D3D11_BIND_VERTEX_BUFFER, colors.data(), static_cast<UINT>(sizeof(ColorVertex) * colors.size()), colorBuffer)) ||
            !CreateBuffer(D3D11_BIND_INDEX_BUFFER, indices.data(), static_cast<UINT>(sizeof(unsigned int) * indices.size()), indexBuffer))
        {
            return false;
        }

        outMesh.SetVertexBuffer(VertexStreamType::Position, std::move(positionBuffer));
        if (hasNormalTangent)
            outMesh.SetVertexBuffer(VertexStreamType::NormalTangent, std::move(normalTangentBuffer));

        if (hasPrimaryTexCoord)
            outMesh.SetVertexBuffer(VertexStreamType::PrimaryTexCoord, std::move(primaryTexCoordBuffer));
        if (hasSecondaryTexCoord)
            outMesh.SetVertexBuffer(VertexStreamType::SecondaryTexCoord, std::move(secondaryTexCoordBuffer));

        if (hasExtraTexCoords)
            outMesh.SetVertexBuffer(VertexStreamType::ExtraTexCoord, std::move(extraTexCoordBuffer));

        if (hasColor)
            outMesh.SetVertexBuffer(VertexStreamType::Color, std::move(colorBuffer));
        outMesh.SetIndexBuffer(std::move(indexBuffer));
        outMesh.materialSlot = sourceMesh.mMaterialIndex;
        outMesh.indexCount = static_cast<unsigned int>(indices.size());
        outMesh.meshID = g_nextMeshID++;
        return true;
    }

    Matrix4x4 ToRowVectorMatrix(const aiMatrix4x4& sourceMatrix)
    {
        // Assimpの列ベクトル行列を、フレームワークの行ベクトル行列へ転置して格納する。
        return Matrix4x4(
            sourceMatrix.a1, sourceMatrix.b1, sourceMatrix.c1, sourceMatrix.d1,
            sourceMatrix.a2, sourceMatrix.b2, sourceMatrix.c2, sourceMatrix.d2,
            sourceMatrix.a3, sourceMatrix.b3, sourceMatrix.c3, sourceMatrix.d3,
            sourceMatrix.a4, sourceMatrix.b4, sourceMatrix.c4, sourceMatrix.d4);
    }

    void CreateNode(
        const aiNode& sourceNode,
        const std::vector<uint32_t>& meshIndices,
        ModelNode& outNode)
    {
        outNode.localTransform = ToRowVectorMatrix(sourceNode.mTransformation);

        for (unsigned int meshIndex = 0; meshIndex < sourceNode.mNumMeshes; ++meshIndex)
        {
            const uint32_t sourceMeshIndex = sourceNode.mMeshes[meshIndex];
            if (sourceMeshIndex < meshIndices.size() && meshIndices[sourceMeshIndex] != UINT_MAX)
                outNode.meshIndices.push_back(meshIndices[sourceMeshIndex]);
        }

        outNode.children.resize(sourceNode.mNumChildren);
        for (unsigned int childIndex = 0; childIndex < sourceNode.mNumChildren; ++childIndex)
        {
            CreateNode(
                *sourceNode.mChildren[childIndex],
                meshIndices,
                outNode.children[childIndex]);
        }
    }
}

bool ModelLoader::Load(const std::string& filePath, ModelAsset& outModelAsset)
{
    if (!IsSupportedModelFile(filePath))
    {
        ShowLoadError("Unsupported model format: " + filePath);
        return false;
    }

    Assimp::Importer importer;
    constexpr unsigned int importFlags = aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals |
        aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFile(filePath, importFlags);
    if (!scene || !scene->HasMeshes())
    {
        ShowLoadError("Failed to import model: " + filePath + "\n" + importer.GetErrorString());
        return false;
    }

    Release(outModelAsset);

    const std::filesystem::path modelDirectory = std::filesystem::path(filePath).parent_path();
    outModelAsset.embeddedMaterials.resize(scene->mNumMaterials);
    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
    {
        aiString sourceMaterialName;
        scene->mMaterials[materialIndex]->Get(AI_MATKEY_NAME, sourceMaterialName);

        const std::string materialName = "Model." + filePath + "." + std::to_string(materialIndex) + "." + sourceMaterialName.C_Str();
        outModelAsset.embeddedMaterials[materialIndex] = CreateMaterial(
            *scene,
            *scene->mMaterials[materialIndex],
            modelDirectory,
            materialName);
    }

    if (outModelAsset.embeddedMaterials.empty() ||
        std::any_of(outModelAsset.embeddedMaterials.begin(), outModelAsset.embeddedMaterials.end(), [](const Material* material) { return material == nullptr; }))
    {
        ShowLoadError("Default shader is not available for model: " + filePath);
        Release(outModelAsset);
        return false;
    }

    // aiMeshは一度だけGPUバッファ化し、Node側は共有するMeshのindexだけを保持する。
    std::vector<uint32_t> meshIndices(scene->mNumMeshes, UINT_MAX);
    for (unsigned int sourceMeshIndex = 0; sourceMeshIndex < scene->mNumMeshes; ++sourceMeshIndex)
    {
        const aiMesh& sourceMesh = *scene->mMeshes[sourceMeshIndex];
        if (sourceMesh.mMaterialIndex >= outModelAsset.embeddedMaterials.size())
            continue;

        Mesh mesh;
        if (!CreateMesh(sourceMesh, mesh))
            continue;

        meshIndices[sourceMeshIndex] = static_cast<uint32_t>(outModelAsset.meshes.size());
        outModelAsset.meshes.push_back(std::move(mesh));
    }

    CreateNode(*scene->mRootNode, meshIndices, outModelAsset.rootNode);

    if (outModelAsset.meshes.empty())
    {
        ShowLoadError("Model does not contain drawable triangles: " + filePath);
        Release(outModelAsset);
        return false;
    }

    return true;
}

void ModelLoader::Release(ModelAsset& modelAsset)
{
    // MaterialはMaterialManagerが所有するため、ModelAsset側では参照とGPU形状だけを解放する。
    for (Mesh& mesh : modelAsset.meshes)
        mesh.Clear();

    modelAsset.meshes.clear();
    modelAsset.embeddedMaterials.clear();
    modelAsset.rootNode = {};
}
