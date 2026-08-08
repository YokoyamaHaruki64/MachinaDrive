/*=============================================================================

File   : [ShaderFactory.cpp]
Desc   : シェーダーの生成を行うクラスの実装

------------------------------------------------------------------------------

Date   : 2026/04/10
Author : Yokoyama Haruki

===============================================================================*/

#include "Graphics/Shader/ShaderFactory.h"
#include "Graphics/Shader/ShaderCompiler.h"
#include "Graphics/Shader/ShaderReflector.h"
#include "Graphics/Shader/InputLayoutBuilder.h"
#include "Graphics/Shader/ShaderInc.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/renderer.h"

bool ShaderFactory::createVertexShader(
    ID3DBlob* shaderBlob,
    ID3D11VertexShader** outVertexShader,
    ID3D11InputLayout** outInputLayout,
    std::vector<InputSlotBinding>& outInputSlot)
{
    HRESULT hr = Renderer::GetDevice()->CreateVertexShader(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        nullptr,
        outVertexShader
    );

    if (FAILED(hr))
    {
        MessageBoxW(NULL, L"Failed to create vertex shader from shader blob.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::vector<InputSlotBinding> inputSlot;
    bool success = m_inputLayoutBuilder.CreateInputLayout(
        shaderBlob,
        inputSlot,
        outInputLayout
    );

    if (!success)
    {
        return false;
    }

    outInputSlot = std::move(inputSlot);

    return true;
}

bool ShaderFactory::createPixelShader(
    ID3DBlob* shaderBlob,
    ID3D11PixelShader** outPixelShader)
{
    HRESULT hr = Renderer::GetDevice()->CreatePixelShader(
        shaderBlob->GetBufferPointer(),
        shaderBlob->GetBufferSize(),
        nullptr,
        outPixelShader
    );

    if (FAILED(hr))
    {
        MessageBoxW(NULL, L"Failed to create pixel shader from shader blob.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

bool ShaderFactory::compileVertexShader(
    const std::string& hlslCode,
    const std::string& entryPoint,
    const std::string& shaderModel,
    ID3DBlob** outVertexShaderBlob)
{

    return (m_shaderCompiler.CompileShader(
        hlslCode,
        entryPoint,
        shaderModel,
        outVertexShaderBlob
    ));
}

bool ShaderFactory::compilePixelShader(
    const std::string& hlslCode,
    const std::string& entryPoint,
    const std::string& shaderModel,
    ID3DBlob** outPixelShaderBlob)
{
    return (m_shaderCompiler.CompileShader(
        hlslCode,
        entryPoint,
        shaderModel,
        outPixelShaderBlob
    ));
}

void ShaderFactory::mergeShaderReflectionResults(
    const ShaderReflector::ReflectionResult& vertexReflectionResult,
    const ShaderReflector::ReflectionResult& pixelReflectionResult,
    std::vector<ConstantBufferInfo>& outConstantBufferInfo,
    std::vector<ShaderTextureInfo>& outTextureInfo,
    std::vector<ShaderSamplerInfo>& outSamplerInfo)
{
    // 定数バッファのマージ
    outConstantBufferInfo = vertexReflectionResult.constantBuffers;
    for (const auto& cbInfo : pixelReflectionResult.constantBuffers)
    {
        // 名前一致による判定
        auto it = std::find_if(outConstantBufferInfo.begin(), outConstantBufferInfo.end(),
            [&cbInfo](const ConstantBufferInfo& existingCbInfo)
            {
                return existingCbInfo.name == cbInfo.name;
            });

        if (it != outConstantBufferInfo.end())
        {
            // 既に存在する定数バッファがある場合は、スロット情報を更新
            it->vsSlot = (it->vsSlot != -1) ? it->vsSlot : cbInfo.vsSlot;
            it->psSlot = (it->psSlot != -1) ? it->psSlot : cbInfo.psSlot;
        }
        else
        {
            outConstantBufferInfo.push_back(cbInfo);
        }
    }

    // テクスチャのマージ
    outTextureInfo = vertexReflectionResult.textures;
    for (const auto& texInfo : pixelReflectionResult.textures)
    {
        auto it = std::find_if(outTextureInfo.begin(), outTextureInfo.end(),
            [&texInfo](const ShaderTextureInfo& existingTexInfo)
            {
                return existingTexInfo.name == texInfo.name;
            });
        if (it != outTextureInfo.end())
        {
            it->vsSlot = (it->vsSlot != -1) ? it->vsSlot : texInfo.vsSlot;
            it->psSlot = (it->psSlot != -1) ? it->psSlot : texInfo.psSlot;
        }
        else
        {
            outTextureInfo.push_back(texInfo);
        }
    }

    // サンプラーのマージ
    outSamplerInfo = vertexReflectionResult.samplers;
    for (const auto& samplerInfo : pixelReflectionResult.samplers)
    {
        auto it = std::find_if(outSamplerInfo.begin(), outSamplerInfo.end(),
            [&samplerInfo](const ShaderSamplerInfo& existingSamplerInfo)
            {
                return existingSamplerInfo.name == samplerInfo.name;
            });
        if (it != outSamplerInfo.end())
        {
            it->vsSlot = (it->vsSlot != -1) ? it->vsSlot : samplerInfo.vsSlot;
            it->psSlot = (it->psSlot != -1) ? it->psSlot : samplerInfo.psSlot;
        }
        else
        {
            outSamplerInfo.push_back(samplerInfo);
        }
    }
}

bool ShaderFactory::createConstantBuffers(const std::vector<ConstantBufferInfo>& constantBufferInfo, std::vector<ComPtr<ID3D11Buffer>>& outConstantBuffers)
{
    outConstantBuffers.reserve(constantBufferInfo.size());

    for (const auto& cbInfo : constantBufferInfo)
    {
        D3D11_BUFFER_DESC bufferDesc = {};
        bufferDesc.ByteWidth = cbInfo.size;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        ComPtr<ID3D11Buffer> constantBuffer;
        HRESULT hr = Renderer::GetDevice()->CreateBuffer(&bufferDesc, nullptr, constantBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            MessageBoxW(NULL, L"Failed to create constant buffer.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        outConstantBuffers.push_back(constantBuffer);
    }
    return true;
}

bool ShaderFactory::createSamplerStates(const std::vector<ShaderSamplerInfo>& samplerInfo, std::vector<ComPtr<ID3D11SamplerState>>& outSamplerStates)
{
    outSamplerStates.reserve(samplerInfo.size());

    for (const auto& sampler : samplerInfo)
    {
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.BorderColor[0] = 0.0f;
        samplerDesc.BorderColor[1] = 0.0f;
        samplerDesc.BorderColor[2] = 0.0f;
        samplerDesc.BorderColor[3] = 0.0f;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
        ComPtr<ID3D11SamplerState> samplerState;
        HRESULT hr = Renderer::GetDevice()->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
        if (FAILED(hr))
        {
            MessageBoxW(NULL, L"Failed to create sampler state.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        outSamplerStates.push_back(samplerState);
    }
    return true;
}

bool ShaderFactory::CreateGraphicsShaderPass(
    const std::string& hlslCode,
    const std::string& vertexEntryPoint,
    const std::string& vertexShaderModel,
    const std::string& pixelEntryPoint,
    const std::string& pixelShaderModel,
    const RenderState& renderState,
    GraphicsShaderPass& outShaderPass)
{
    // ===========================
    // シェーダーのコンパイル
    // ===========================

    // 頂点シェーダー
    ComPtr<ID3DBlob> vertexShaderBlob;
    if (!compileVertexShader(
        hlslCode,
        vertexEntryPoint,
        vertexShaderModel,
        vertexShaderBlob.GetAddressOf()))
    {
        MessageBoxW(NULL, L"Failed to compile vertex shader.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    // ピクセルシェーダー
    ComPtr<ID3DBlob> pixelShaderBlob;
    if (!compilePixelShader(
        hlslCode,
        pixelEntryPoint,
        pixelShaderModel,
        pixelShaderBlob.GetAddressOf()))
    {
        MessageBoxW(NULL, L"Failed to compile pixel shader.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // ===========================
    // シェーダーオブジェクトの生成
    // ===========================

    // 頂点シェーダーと入力レイアウトの生成
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11InputLayout> inputLayout;
    std::vector<InputSlotBinding> inputSlot;
    if (!createVertexShader(
        vertexShaderBlob.Get(),
        vertexShader.GetAddressOf(),
        inputLayout.GetAddressOf(),
        inputSlot))
    {
        MessageBoxW(NULL, L"Failed to create vertex shader and input layout.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // ピクセルシェーダーの生成
    ComPtr<ID3D11PixelShader> pixelShader;
    if (!createPixelShader(pixelShaderBlob.Get(), pixelShader.GetAddressOf()))
    {
        MessageBoxW(NULL, L"Failed to create pixel shader.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // ===========================
    // シェーダーのリフレクション
    // ===========================

    // 頂点シェーダーのリフレクション
    ShaderReflector vertexReflector(vertexShaderBlob.Get(), ShaderStage::Vertex);
    ShaderReflector::ReflectionResult vertexReflectionResult;
    if (!vertexReflector.ReflectShader(vertexReflectionResult))
    {
        MessageBoxW(NULL, L"Failed to reflect vertex shader.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // ピクセルシェーダーのリフレクション
    ShaderReflector pixelReflector(pixelShaderBlob.Get(), ShaderStage::Pixel);
    ShaderReflector::ReflectionResult pixelReflectionResult;
    if (!pixelReflector.ReflectShader(pixelReflectionResult))
    {
        MessageBoxW(NULL, L"Failed to reflect pixel shader.", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // 頂点シェーダーとピクセルシェーダーのリフレクション結果をマージする
    std::vector<ConstantBufferInfo> constantBufferInfo;
    std::vector<ShaderTextureInfo> textureInfo;
    std::vector<ShaderSamplerInfo> samplerInfo;
    mergeShaderReflectionResults(
        vertexReflectionResult,
        pixelReflectionResult,
        constantBufferInfo,
        textureInfo,
        samplerInfo);

    // ===========================
    // シェーダーパスの生成
    // ===========================
    outShaderPass = GraphicsShaderPass(
        std::move(vertexShader),
        std::move(pixelShader),
        std::move(inputLayout),
        renderState,
        std::move(inputSlot),
        std::move(constantBufferInfo),
        std::move(textureInfo),
        std::move(samplerInfo)
    );

    return true;
}
