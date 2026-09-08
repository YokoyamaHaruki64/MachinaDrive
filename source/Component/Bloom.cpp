#include "pch.h"

#include "Component/Bloom.h"

#include "Graphics/Material.h"
#include "Graphics/TextureManager.h"
#include "System/Rendering/renderer.h"
#include "Graphics/Shader/ShaderProgram.h"

Bloom::Bloom(GameObject* owner)
    : PostProcess(owner)
{
    const UINT bloomWidth = std::max<UINT>(Renderer::GetGameWidth() / 2, 1);
    const UINT bloomHeight = std::max<UINT>(Renderer::GetGameHeight() / 2, 1);
    m_bloomTexture = TextureManager::CreateRenderTexture("PostProcess/Bloom", bloomWidth, bloomHeight);
    if (m_bloomTexture.IsValid())
        OnMaterialChanged();
}

void Bloom::OnMaterialChanged()
{
    if (Material* material = GetMaterial())
    {
        material->SetTexture("_Bloom", m_bloomTexture);
        m_bloomExtractPassIndex = material->GetShader().GetShaderPassIndex("BloomExtract").value_or(-1);
        m_compositePassIndex = material->GetShader().GetShaderPassIndex("Composite").value_or(-1);
    }
}

void Bloom::Resize(UINT width, UINT height)
{
    const TextureHandle resizedTexture = TextureManager::CreateRenderTexture(
        "PostProcess/Bloom",
        std::max<UINT>(width / 2, 1),
        std::max<UINT>(height / 2, 1));
    if (!resizedTexture.IsValid())
        return;

    m_bloomTexture = resizedTexture;
    OnMaterialChanged();
}

void Bloom::Apply()
{
    if (!m_bloomTexture.IsValid() || !GetMaterial())
        return;

    if (!ExecuteMiddlePass(m_bloomExtractPassIndex, m_bloomTexture))
        return;

    ExecuteScreenPassAndSwap(m_compositePassIndex);
}
