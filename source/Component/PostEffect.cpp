#include "pch.h"

#include "Component/ComponentInc.h"


#include "Component/PostEffect.h"

#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include "System/Rendering/renderer.h"

Microsoft::WRL::ComPtr<ID3D11Buffer> PostProcess::m_positionBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> PostProcess::m_texCoordBuffer;

namespace
{
    template <typename VertexType>
    Microsoft::WRL::ComPtr<ID3D11Buffer> CreateFullscreenVertexBuffer(const std::vector<VertexType>& vertices)
    {
        D3D11_BUFFER_DESC description{};
        description.ByteWidth = static_cast<UINT>(sizeof(VertexType) * vertices.size());
        description.Usage = D3D11_USAGE_IMMUTABLE;
        description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = vertices.data();

        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        if (FAILED(Renderer::GetDevice()->CreateBuffer(&description, &initialData, buffer.GetAddressOf())))
            MessageBoxA(nullptr, "ポストプロセス用頂点バッファの作成に失敗しました。", "Post Process Error", MB_OK | MB_ICONERROR);

        return buffer;
    }
}

void PostProcess::Init()
{
    const std::vector<PositionVertex> positions
    {
        { Vector3(-1.0f, 1.0f, 0.0f) }, { Vector3(1.0f, 1.0f, 0.0f) },
        { Vector3(-1.0f, -1.0f, 0.0f) }, { Vector3(1.0f, -1.0f, 0.0f) },
    };
    const std::vector<PrimaryTexCoordVertex> texCoords
    {
        { Vector4(0.0f, 0.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 0.0f, 0.0f, 0.0f) },
        { Vector4(0.0f, 1.0f, 0.0f, 0.0f) }, { Vector4(1.0f, 1.0f, 0.0f, 0.0f) },
    };

    m_positionBuffer = CreateFullscreenVertexBuffer(positions);
    m_texCoordBuffer = CreateFullscreenVertexBuffer(texCoords);
}

void PostProcess::Uninit()
{
    m_positionBuffer.Reset();
    m_texCoordBuffer.Reset();
}

PostProcess::PostProcess(const char* shaderFilePath)
{
    // PostProcessごとの値は独立したMaterialへ持たせ、Property APIを通常描画と共用する。
    static uint32_t materialInstanceIndex = 0;
    if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile(shaderFilePath))
        m_material = MaterialManager::Create("PostProcess." + std::to_string(materialInstanceIndex++), *shader);
}

bool PostProcess::BindFullscreenVertices(const GraphicsShaderPass& shaderPass) const
{
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    for (const InputSlotBinding& binding : shaderPass.GetInputSlotBindings())
    {
        ID3D11Buffer* vertexBuffer{};
        switch (binding.streamType)
        {
        case VertexStreamType::Position:
            vertexBuffer = m_positionBuffer.Get();
            break;
        case VertexStreamType::PrimaryTexCoord:
            vertexBuffer = m_texCoordBuffer.Get();
            break;
        default:
            vertexBuffer = Renderer::GetFallbackVertexBuffer(binding.streamType);
            break;
        }

        const UINT stride = Mesh::GetVertexStreamStride(binding.streamType);
        const UINT offset = 0;
        if (!vertexBuffer || stride == 0)
            return false;

        context->IASetVertexBuffers(binding.inputSlot, 1, &vertexBuffer, &stride, &offset);
    }

    return true;
}

void PostProcess::BindSourceTexture() const
{
    // 画面入力はPropertyではなく、すべてのPostProcessで共通のt0へ設定する。
    ID3D11ShaderResourceView* source = Renderer::GetSrcSRV();
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &source);
}

void PostProcess::ClearBoundTextures(const GraphicsShaderPass& shaderPass) const
{
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    ID3D11ShaderResourceView* nullResource = nullptr;

    // 次のRTとの読み書き競合を避けるため、このPassが設定したSRVを解除する。
    context->PSSetShaderResources(0, 1, &nullResource);
    for (const ShaderTextureInfo& binding : shaderPass.GetTextureInfo())
    {
        if (binding.vsSlot >= 0)
            context->VSSetShaderResources(binding.vsSlot, 1, &nullResource);
        if (binding.psSlot >= 0)
            context->PSSetShaderResources(binding.psSlot, 1, &nullResource);
    }
}

bool PostProcess::ExecutePass(size_t passIndex)
{
    if (!m_material || passIndex >= m_material->GetShader().GetShaderPasses().size())
        return false;

    const GraphicsShaderPass& shaderPass = m_material->GetShaderPass(passIndex);

    // 画面出力では、現在のSourceと異なるDstへ描画して完了後にSwapする。
    Renderer::SetDstRTV();
    m_material->Bind(passIndex);
    BindSourceTexture();
    const bool canDraw = BindFullscreenVertices(shaderPass);
    if (canDraw)
        DrawFullscreen();

    ClearBoundTextures(shaderPass);
    if (!canDraw)
        return false;

    Renderer::SwapBuffers();
    return true;
}

bool PostProcess::ExecutePass(std::string_view passTag)
{
    if (!m_material)
        return false;

    const std::optional<size_t> passIndex = m_material->GetShader().GetShaderPassIndex(passTag);
    return passIndex && ExecutePass(*passIndex);
}

bool PostProcess::ExecutePass(size_t passIndex, TextureHandle renderTarget)
{
    if (!m_material || passIndex >= m_material->GetShader().GetShaderPasses().size())
        return false;

    const GraphicsShaderPass& shaderPass = m_material->GetShaderPass(passIndex);
    if (!Renderer::SetRenderTarget(renderTarget))
        return false;

    // 中間RTへの出力ではSwapせず、次のPassも同じ画面入力を参照できるようにする。
    m_material->Bind(passIndex);
    BindSourceTexture();
    const bool canDraw = BindFullscreenVertices(shaderPass);
    if (canDraw)
        DrawFullscreen();

    ClearBoundTextures(shaderPass);
    return canDraw;
}

bool PostProcess::ExecutePass(std::string_view passTag, TextureHandle renderTarget)
{
    if (!m_material)
        return false;

    const std::optional<size_t> passIndex = m_material->GetShader().GetShaderPassIndex(passTag);
    return passIndex && ExecutePass(*passIndex, renderTarget);
}

void PostProcess::DrawFullscreen() const
{
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->Draw(4, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

Bloom::Bloom()
    : PostProcess(SHADER_FILE_PATH)
{
    // 中間RTはTextureManagerが所有し、Material Propertyへ共通Handleで渡す。
    m_bloomTexture = TextureManager::CreateRenderTexture("PostProcess/Bloom", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    if (!m_bloomTexture.IsValid())
        return;

    if (Material* material = GetMaterial())
    {
        material->SetTexture("_Bloom", m_bloomTexture);
        m_bloomExtractPassIndex = material->GetShader().GetShaderPassIndex("BloomExtract").value_or(-1);
        m_compositePassIndex = material->GetShader().GetShaderPassIndex("Composite").value_or(-1);
    }

}

void Bloom::Apply()
{
    if (!m_bloomTexture.IsValid() || !GetMaterial())
        return;

    // 中間RTへ抽出してから、Materialの_Bloom Propertyを使って画面へ合成する。
    if (!ExecutePass(m_bloomExtractPassIndex, m_bloomTexture))
        return;

    ExecutePass(m_compositePassIndex);
}
