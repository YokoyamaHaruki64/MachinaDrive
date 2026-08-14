#include "pch.h"

#include "Component/modelRenderer.h"

#include "Component/Transform.h"
#include "Graphics/Material.h"
#include "Graphics/ModelLoader.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/renderer.h"

#include "math/MathCommon.h"

#include <unordered_set>

std::unordered_map<std::string, ModelAsset*> ModelRenderer::m_ModelPool;

namespace
{
    std::unordered_set<uint32_t> g_reportedMissingPositionMeshes;

    XMMATRIX ToXMMATRIX(const Matrix4x4& matrix)
    {
        return XMMATRIX(
            matrix.m[0][0], matrix.m[0][1], matrix.m[0][2], matrix.m[0][3],
            matrix.m[1][0], matrix.m[1][1], matrix.m[1][2], matrix.m[1][3],
            matrix.m[2][0], matrix.m[2][1], matrix.m[2][2], matrix.m[2][3],
            matrix.m[3][0], matrix.m[3][1], matrix.m[3][2], matrix.m[3][3]);
    }

    void DrawMesh(const ModelRenderer& renderer, const Mesh& mesh, const XMMATRIX& worldMatrix)
    {
        ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();
        deviceContext->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

        Material* material = renderer.GetMaterial(mesh.materialSlot);
        if (!material)
            return;

        Renderer::SetWorldMatrix(worldMatrix);
        material->Bind(0);
        const GraphicsShaderPass& shaderPass = material->GetShaderPass(0);

        // Passが要求するStreamだけを、その入力スロットへ対応付ける。
        bool hasAllRequiredStreams = true;
        for (const InputSlotBinding& binding : shaderPass.GetInputSlotBindings())
        {
                ID3D11Buffer* vertexBuffer = mesh.GetVertexBuffer(binding.streamType);
                const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
                if (stride == 0)
                {
                    hasAllRequiredStreams = false;
                    break;
                }

                if (!vertexBuffer)
                {
                    if (binding.streamType == VertexStreamType::Position)
                    {
                        // Positionの欠落は形状を復元できないため、既定値で補完せず一度だけ通知する。
                        if (g_reportedMissingPositionMeshes.insert(mesh.meshID).second)
                        {
                            MessageBoxA(nullptr, "Mesh does not contain a Position vertex stream.",
                                "Model Renderer Error", MB_OK | MB_ICONERROR);
                        }

                        hasAllRequiredStreams = false;
                        break;
                    }

                    vertexBuffer = Renderer::GetFallbackVertexBuffer(binding.streamType);
                    if (!vertexBuffer)
                    {
                        hasAllRequiredStreams = false;
                        break;
                    }
                }

            const UINT offset = 0;
            deviceContext->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
        }

        if (hasAllRequiredStreams)
            deviceContext->DrawIndexed(mesh.indexCount, 0, 0);
    }

    void DrawNode(const ModelRenderer& renderer, const ModelAsset& modelAsset, const ModelNode& node, const XMMATRIX& parentWorldMatrix)
    {
        // 行ベクトル系のため、ローカル変換を親変換の左側へ積算する。
        const XMMATRIX worldMatrix = ToXMMATRIX(node.localTransform) * parentWorldMatrix;
        for (const uint32_t meshIndex : node.meshIndices)
        {
            if (meshIndex < modelAsset.meshes.size())
                DrawMesh(renderer, modelAsset.meshes[meshIndex], worldMatrix);
        }

        for (const ModelNode& child : node.children)
            DrawNode(renderer, modelAsset, child, worldMatrix);
    }
}

void ModelRenderer::Load(
    const char* modelFileName,
    const char* /*vertexShaderFileName*/,
    const char* /*pixelShaderFileName*/)
{
    if (const auto foundModel = m_ModelPool.find(modelFileName); foundModel != m_ModelPool.end())
    {
        m_model = foundModel->second;
        m_materialOverrides.assign(m_model->embeddedMaterials.size(), nullptr);
        return;
    }

    auto* model = new ModelAsset;
    if (!ModelLoader::Load(modelFileName, *model))
    {
        delete model;
        return;
    }

    m_model = model;
    m_materialOverrides.assign(m_model->embeddedMaterials.size(), nullptr);
    m_ModelPool.emplace(modelFileName, model);
}

Material* ModelRenderer::ResolveMaterial(uint32_t materialSlot) const
{
    // Renderer固有の差し替えがある場合だけ優先し、通常はModelAssetの既定Materialを使う。
    if (materialSlot < m_materialOverrides.size() && m_materialOverrides[materialSlot])
        return m_materialOverrides[materialSlot];

    if (!m_model || materialSlot >= m_model->embeddedMaterials.size())
        return nullptr;

    return m_model->embeddedMaterials[materialSlot];
}

void ModelRenderer::SetMaterial(uint32_t materialSlot, Material* material)
{
    if (!m_model || materialSlot >= m_materialOverrides.size())
        return;

    // nullptrを設定すると、ファイルに埋め込まれた既定Materialへ戻す。
    m_materialOverrides[materialSlot] = material;
}

Material* ModelRenderer::GetMaterial(uint32_t materialSlot) const
{
    return ResolveMaterial(materialSlot);
}

void ModelRenderer::Draw()
{
    if (!m_model)
        return;

    ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const XMMATRIX axisInverse = XMMatrixRotationRollPitchYaw(
        m_inverseAxisFlags & InverseAxis::X ? Math::PI : 0.0f,
        m_inverseAxisFlags & InverseAxis::Y ? Math::PI : 0.0f,
        m_inverseAxisFlags & InverseAxis::Z ? Math::PI : 0.0f);
    const XMMATRIX componentWorldMatrix = transform() ? CalcWorldMatrix() : XMMatrixIdentity();

    // ModelAssetのNode階層を辿り、各MeshをComponentのWorld座標へ配置して描画する。
    DrawNode(*this, *m_model, m_model->rootNode, axisInverse * componentWorldMatrix);
}

void ModelRenderer::Preload(const char* fileName)
{
    if (m_ModelPool.contains(fileName))
        return;

    auto* model = new ModelAsset;
    if (!ModelLoader::Load(fileName, *model))
    {
        delete model;
        return;
    }

    m_ModelPool.emplace(fileName, model);
}

void ModelRenderer::UnloadAll()
{
    for (const auto& [fileName, model] : m_ModelPool)
    {
        ModelLoader::Release(*model);
        delete model;
    }

    m_ModelPool.clear();
}
