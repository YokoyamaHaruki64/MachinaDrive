#include "pch.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/renderer.h"

void GraphicsShaderPass::Bind() const
{
    ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();

    // PassがReflection時に確定した入力形式とShaderをまとめて切り替える。
    deviceContext->IASetInputLayout(m_inputLayout.Get());
    deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // Shaderファイルに記述した描画ステートを、旧RendererBaseの設定より優先する。
    Renderer::BindRenderState(m_renderState);
}

const ShaderPropertyDesc* GraphicsShader::GetProperty(ShaderPropertyID id) const
{
    // Property IDはParserが宣言順に採番するため、配列の添字として解決できる。
    if (id >= m_properties.size())
        return nullptr;

    return &m_properties[id];
}

const ShaderPropertyDesc* GraphicsShader::GetProperty(std::string_view name) const
{
    // 名前指定APIでは、Shader生成時に作成した名前→IDテーブルを使って線形探索を避ける。
    const auto found = m_propertyIDsByName.find(std::string(name));
    return found == m_propertyIDsByName.end() ? nullptr : GetProperty(found->second);
}
