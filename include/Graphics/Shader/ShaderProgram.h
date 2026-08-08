/*=============================================================================
File   : [ShaderProgram.h]
Desc   : シェーダーの管理を行うクラスの宣言

------------------------------------------------------------------------------

Date   : 2026/03/12
Author : Yokoyama Haruki

===============================================================================*/

#ifndef _SHADER_PROGRAM_H_
#define _SHADER_PROGRAM_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <d3d11.h>
#include <wrl/client.h>
#include "Graphics/Shader/ShaderInc.h"
#include "Graphics/GraphicInc.h"

using Microsoft::WRL::ComPtr;


/// @brief 描画用シェーダークラス
/// シェーダーのリソースと情報を持つクラス
class GraphicsShaderPass
{
private:
    ComPtr<ID3D11VertexShader> m_vertexShader{ nullptr };
    ComPtr<ID3D11PixelShader> m_pixelShader{ nullptr };
    ComPtr<ID3D11InputLayout> m_inputLayout{ nullptr };

    RenderState m_renderState{};

    std::vector<InputSlotBinding> m_bindings;
    std::vector<ConstantBufferInfo> m_constantBufferInfo{};
    std::vector<ShaderTextureInfo> m_textureInfo{};
    std::vector<ShaderSamplerInfo> m_samplerInfo{};

public:

    GraphicsShaderPass() = default;

    GraphicsShaderPass(
        ComPtr<ID3D11VertexShader> vertexShader,
        ComPtr<ID3D11PixelShader> pixelShader,
        ComPtr<ID3D11InputLayout> inputLayout,
        RenderState renderState,
        std::vector<InputSlotBinding> bindings,
        std::vector<ConstantBufferInfo> constantBufferInfo,
        std::vector<ShaderTextureInfo> textureInfo,
        std::vector<ShaderSamplerInfo> samplerInfo
    )
        :
        m_vertexShader(std::move(vertexShader)),
        m_pixelShader(std::move(pixelShader)),
        m_inputLayout(std::move(inputLayout)),
        m_renderState(renderState),
        m_bindings(std::move(bindings)),
        m_constantBufferInfo(std::move(constantBufferInfo)),
        m_textureInfo(std::move(textureInfo)),
        m_samplerInfo(std::move(samplerInfo))
    {
    }

    // このシェーダーが正当なものであるかをチェックする関数
    bool IsValid() const
    {
        return m_vertexShader != nullptr && m_pixelShader != nullptr;
    }
};

class GraphicsShader
{
    ShaderID m_id{ 0 };

    std::string m_shaderName{};
    std::string m_filePath{};

    std::vector<GraphicsShaderPass> m_shaderPasses{};
    std::vector<ShaderPropertyDesc> m_properties{};

public:
    GraphicsShader(
        ShaderID id,
        const std::string& shaderName,
        const std::string& filePath,
        std::vector<GraphicsShaderPass> shaderPasses,
        std::vector<ShaderPropertyDesc> properties)
        :
        m_id(id),
        m_shaderName(shaderName),
        m_filePath(filePath),
        m_shaderPasses(std::move(shaderPasses)),
        m_properties(std::move(properties))
    {
    }
    const ShaderID& GetID() const { return m_id; }
    const std::string& GetShaderName() const { return m_shaderName; }
    const std::string& GetFilePath() const { return m_filePath; }
    const std::vector<GraphicsShaderPass>& GetShaderPasses() const { return m_shaderPasses; }
    const std::vector<ShaderPropertyDesc>& GetProperties() const { return m_properties; }

    bool IsValid() const
    {
        return !m_shaderPasses.empty();
    }
};

#endif // _SHADER_PROGRAM_H_
