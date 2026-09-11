#include "pch.h"

#include "Component/ComponentInc.h"


#include "Component/modelRenderer.h"

#include "Graphics/Material.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/Rendering/renderer.h"

#include <unordered_set>

ModelRenderer::ModelRenderer(GameObject* object)
    : RendererBase(object)
{
}

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

    void DrawMesh(ModelRenderer& renderer, const Mesh& mesh, const XMMATRIX& worldMatrix, RenderPhase phase)
    {
        ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();
        deviceContext->IASetIndexBuffer(mesh.GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

        Material* material = renderer.GetMaterial(mesh.materialSlot);
        if (!material)
            return;

        if (phase == RenderPhase::Shadow && !renderer.GetCastShadow())
            return;

        const std::vector<GraphicsShaderPass>& shaderPasses = material->GetShader().GetShaderPasses();
        for (size_t passIndex = 0; passIndex < shaderPasses.size(); ++passIndex)
        {
            const GraphicsShaderPass& shaderPass = shaderPasses[passIndex];
            if (shaderPass.GetRenderState().renderPhase != phase)
                continue;

            Renderer::SetWorldMatrix(worldMatrix);
            Renderer::SetReceiveShadow(renderer.GetReceiveShadow());
            material->Bind(passIndex, renderer.FindMaterialPropertyOverride(mesh.materialSlot));

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
    }

    void DrawNode(ModelRenderer& renderer, const ModelAsset& modelAsset, const ModelNode& node, const XMMATRIX& parentWorldMatrix, RenderPhase phase)
    {
        // 行ベクトル系のため、ローカル変換を親変換の左側へ積算する。
        const XMMATRIX worldMatrix = ToXMMATRIX(node.localTransform) * parentWorldMatrix;
        for (const uint32_t meshIndex : node.meshIndices)
        {
            if (meshIndex < modelAsset.meshes.size())
                DrawMesh(renderer, modelAsset.meshes[meshIndex], worldMatrix, phase);
        }

        for (const ModelNode& child : node.children)
            DrawNode(renderer, modelAsset, child, worldMatrix, phase);
    }
}

ModelRenderer::ModelRenderer(
    GameObject* object,
    const char* modelFileName,
    uint8_t inverseAxisFlags)
    : RendererBase(object),
    m_modelPath(modelFileName ? std::filesystem::path(modelFileName).generic_string() : std::string{}),
    m_inverseAxisFlags(inverseAxisFlags)
{
    SetModel(modelFileName ? ModelManager::Load(modelFileName) : nullptr);
}

ModelRenderer::ModelRenderer(
    GameObject* object,
    ModelAsset* modelAsset,
    uint8_t inverseAxisFlags)
    : RendererBase(object),
    m_inverseAxisFlags(inverseAxisFlags)
{
    m_modelPath = ModelManager::GetSourcePath(modelAsset).value_or(std::string{});
    SetModel(modelAsset);
}

void ModelRenderer::OnLoaded()
{
    if (!m_model && !m_modelPath.empty())
    {
        SetModel(ModelManager::Load(m_modelPath));
    }

    m_loadedModelPath = m_modelPath;

    for (size_t index = 0; index < m_materialPaths.size() && index < m_materialOverrides.size(); ++index)
    {
        if (!m_materialPaths[index].empty())
        {
            m_materialOverrides[index] = MaterialManager::Load(m_materialPaths[index]);
        }
    }
}

void ModelRenderer::OnEditorUpdate()
{
    if (m_loadedModelPath == m_modelPath)
        return;

    m_model = nullptr;
    m_materialOverrides.clear();
    OnLoaded();
}

void ModelRenderer::RegisterFields(FieldRegistry& fields)
{
    for (size_t index = 0; index < m_materialOverrides.size(); ++index)
    {
        if (Material* material = m_materialOverrides[index]; material && !material->GetFilePath().empty())
        {
            m_materialPaths[index] = material->GetFilePath();
        }
    }

    RendererBase::RegisterFields(fields);
    GM_FIELD_PATH(m_modelPath, "Assets/Local/Models", ".obj", ".fbx", ".gltf");
    GM_FIELD_TYPE(m_materialPaths, InspectorFieldType::Hidden);
    GM_FIELD(m_inverseAxisFlags);
    GM_FIELD(m_castShadow);
    GM_FIELD(m_receiveShadow);
}

void ModelRenderer::SetModel(ModelAsset* modelAsset)
{
    m_model = modelAsset;
    m_materialPropertyOverrides.clear();
    if (const std::optional<std::string> sourcePath = ModelManager::GetSourcePath(modelAsset))
    {
        m_modelPath = *sourcePath;
    }

    // ModelAssetを差し替えた時点で旧slotのoverrideを破棄し、新しい既定Material構成へ同期する。
    InitializeMaterialOverrides();
}

void ModelRenderer::InitializeMaterialOverrides()
{
    m_materialOverrides.clear();
    if (!m_model)
        return;

    m_materialOverrides.assign(m_model->embeddedMaterials.size(), nullptr);
    m_materialPaths.resize(m_model->embeddedMaterials.size());
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
    m_materialPaths[materialSlot] = material ? material->GetFilePath() : std::string{};
    ClearMaterialPropertyOverride(materialSlot);
}

Material* ModelRenderer::GetMaterial(uint32_t materialSlot)
{
    return ResolveMaterial(materialSlot);
}

uint32_t ModelRenderer::GetMeshID() const
{
    // ModelRendererは複数Meshを描けるため、提出段階では先頭Meshを代表値として使う。
    return m_model && !m_model->meshes.empty() ? m_model->meshes.front().meshID : 0;
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
    DrawNode(*this, *m_model, m_model->rootNode, axisInverse * componentWorldMatrix, Renderer::GetCurrentRenderPhase());
}

void ModelRenderer::Submit()
{
    if (!m_model)
        return;

    std::unordered_set<RenderPhase> phases;
    for (const Mesh& mesh : m_model->meshes)
    {
        Material* material = GetMaterial(mesh.materialSlot);
        if (!material)
            continue;

        for (const GraphicsShaderPass& shaderPass : material->GetShader().GetShaderPasses())
        {
            const RenderPhase phase = shaderPass.GetRenderState().renderPhase;
            if (phase != RenderPhase::PostProcess)
                phases.insert(phase);
        }
    }

    for (size_t phaseIndex = 0; phaseIndex < static_cast<size_t>(RenderPhase::Count); ++phaseIndex)
    {
        const RenderPhase phase = static_cast<RenderPhase>(phaseIndex);
        if (!phases.contains(phase))
            continue;

        DrawItem item{};
        item.renderer = this;
        item.meshID = GetMeshID();
        item.sortKey = static_cast<uint64_t>(GetDrawLayer() + 32768);
        Renderer::SubmitDrawCommand(phase, item);
    }
}
