/*=============================================================================

File   : [ShaderLoader.cpp]
Desc   : シェーダーの読み込みとコンパイルを行うクラスの実装

------------------------------------------------------------------------------

Date   : 2026/03/12
Author : Yokoyama Haruki

===============================================================================*/

#include "Graphics/Shader/ShaderLoader.h"
#include "Graphics/Shader/ShaderParser.h"
#include "Graphics/Shader/ShaderToken.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/Shader/ShaderFactory.h"
#include "Graphics/Shader/ShaderInc.h"

#include <fstream>
#include <unordered_set>
#include <windows.h>

bool ShaderLoader::ResolvePropertyLayout(
    std::vector<ShaderPropertyDesc>& properties,
    const GraphicsShaderPass& shaderPass) const
{
    // PassのReflectionからProperties定数バッファを探す。
    const ConstantBufferInfo* propertiesBuffer = nullptr;
    for (const ConstantBufferInfo& constantBuffer : shaderPass.GetConstantBufferInfo())
    {
        if (constantBuffer.name == "Properties")
        {
            propertiesBuffer = &constantBuffer;
            break;
        }
    }

    // Propertiesを参照しないPassでは、コンパイラが定数バッファを除去する場合がある。
    if (!propertiesBuffer)
        return true;

    for (ShaderPropertyDesc& property : properties)
    {
        // TextureなどのResourceは定数バッファ内にレイアウトを持たない。
        if (property.storage != MaterialPropertyStorage::ConstantBuffer)
            continue;

        const ShaderParameterDesc* parameter = nullptr;
        for (const ShaderParameterDesc& candidate : propertiesBuffer->parameters)
        {
            if (candidate.name == property.name)
            {
                parameter = &candidate;
                break;
            }
        }

        // Pass内で未使用のPropertyはコンパイラに除去される。ほかのPassで得たレイアウトを維持する。
        if (!parameter)
            continue;

        // 複数Passでは全Passのoffset・sizeが一致することを読み込み時に検証する。
        if (property.offset != INVALID_PROPERTY_OFFSET &&
            (property.offset != parameter->offset || property.size != parameter->size))
        {
            return false;
        }

        // 最初にPropertiesを参照したPassのReflection結果をShader共通レイアウトとして確定する。
        property.offset = parameter->offset;
        property.size = parameter->size;
    }

    return true;
}

GraphicsShader* ShaderLoader::LoadShaderFromFile(const std::string& filePath, ShaderID shaderID)
{
    // ファイルからシェーダーコードを読み込む
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        MessageBoxW(NULL, L"Failed to open shaderPass file.", L"Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }
    std::string sourceCode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // シェーダーコードを解析して、必要な情報を抽出する
    ShaderParser parser(sourceCode);

    // シェーダーの名前を抽出
    std::string shaderName;
    if (!parser.ExtractShaderName(shaderName))
    {
        MessageBoxW(NULL, L"Failed to extract shaderPass name from shaderPass source.", L"Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    //// 同名のシェーダーが既に存在するか確認
    //if (m_shaderManager.GetShaderByFileName(shaderName) != nullptr)
    //{
    //    MessageBoxW(NULL, L"GraphicsShaderPass with the same name already exists.", L"Error", MB_OK | MB_ICONERROR);
    //    return nullptr;
    //}

    // Shader全体のRenderStateを抽出
    RenderState defaultRenderState;
    RenderPhase defaultRenderPhase;
    parser.ExtractStateCommand(defaultRenderState, 1);
    parser.ExtractRenderPhase(defaultRenderPhase, 1);

    // シェーダープロパティの抽出
    std::vector<ShaderPropertyDesc> properties;
    parser.ExtractProperties(properties);

    // Parserの宣言から、Material Properties用のHLSL宣言を生成する。
    std::string additionalCode = "#define MATERIAL_PROPERTIES_DEFINED\ncbuffer Properties : register(b3)\n{\n";
    // プロパティの定数バッファの内容を生成
    for (auto& prop : properties)
    {
        if (prop.type == PropertyType::Texture2D)
            continue;

        additionalCode += "    " + std::string(GetPropertyTypeStringForHLSL(prop.type)) + " " + prop.name + ";\n";
    }
    additionalCode += "}\n";

    // Resource Propertyはb3に含めず、予約済みのテクスチャスロットへ個別に宣言する。
    int textureRegisterIndex = RESERVE_TEXTURE_SLOT_COUNT;
    for( auto& prop : properties)
    {
        if (prop.type != PropertyType::Texture2D)
            continue;

        additionalCode += "Texture2D " + prop.name + " : register(t" + std::to_string(textureRegisterIndex++) + ");\n";
    }

    // Passブロックの解析
    std::vector<GraphicsShaderPass> shaderPasses;

    // Passブロックを抽出
    std::string passBlock;
    std::string passTag;
    int passIndex = 0;
    std::unordered_set<std::string> passTags;
    while (parser.ExtractPassBlock(passBlock, passTag, passIndex++))
    {
        // Passブロックの解析
        ShaderParser passParser(passBlock);

        // PassブロックのRenderStateを抽出
        RenderState overrideRenderState = defaultRenderState;
        RenderPhase overrideRenderPhase = defaultRenderPhase;
        passParser.ExtractStateCommand(overrideRenderState);
        passParser.ExtractRenderPhase(overrideRenderPhase);

        // RenderPhaseを確定
        overrideRenderState.renderPhase = overrideRenderPhase;

        // HLSLコードを抽出
        std::string hlslCode;
        if (!passParser.ExtractHLSL(hlslCode))
        {
            MessageBoxW(NULL, L"Failed to extract HLSL code from shaderPass source.", L"Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }

        // 生成したPropertiesとResourceの宣言をHLSL本体の前へ挿入する。
        hlslCode = additionalCode + hlslCode;

        // 頂点シェーダーとピクセルシェーダーのエントリーポイントを抽出
        std::string vertexShaderEntry;
        std::string pixelShaderEntry;
        passParser.ExtractPragma(ShaderToken::PRAGMA_VERTEX_SHADER, vertexShaderEntry);
        passParser.ExtractPragma(ShaderToken::PRAGMA_PIXEL_SHADER, pixelShaderEntry);

        // エントリーポイントが両方とも指定されていない場合はエラー
        if (vertexShaderEntry.empty() && pixelShaderEntry.empty())
        {
            MessageBoxW(NULL, L"GraphicsShaderPass entry points not specified in shaderPass source.", L"Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }

        // シェーダーモデルを抽出
        std::string shaderModel;
        passParser.ExtractPragma(ShaderToken::PRAGMA_SHADER_MODEL, shaderModel);

        // シェーダーモデルが指定されていない場合はデフォルトを使用する
        std::string shaderModelVS = DEFAULT_SHADER_MODEL_VS;
        std::string shaderModelPS = DEFAULT_SHADER_MODEL_PS;
        if (!shaderModel.empty())
        {
            // n.mの形式をパースして、シェーダーモデルを設定する
            size_t dotPos = shaderModel.find('.');
            if (dotPos != std::string::npos)
            {
                std::string majorStr = shaderModel.substr(0, dotPos);
                std::string minorStr = shaderModel.substr(dotPos + 1);
                // 文字列を整数に変換して、シェーダーモデルを構築する
                int major, minor = 0;
                try
                {
                    major = std::stoi(majorStr);
                    minor = std::stoi(minorStr);
                }
                catch (const std::exception&)
                {
                    MessageBoxW(NULL, L"Invalid shaderPass model format. Expected 'n.m' where n and m are integers.", L"Error", MB_OK | MB_ICONERROR);
                    return nullptr;
                }
                shaderModelVS = "vs_" + std::to_string(major) + '_' + std::to_string(minor);
                shaderModelPS = "ps_" + std::to_string(major) + '_' + std::to_string(minor);
            }
            else
            {
                MessageBoxW(NULL, L"Invalid shaderPass model format. Expected 'n.m'.", L"Error", MB_OK | MB_ICONERROR);
                return nullptr;
            }
        }


        // シェーダーパスの生成
        GraphicsShaderPass shaderPass;
        if (!m_shaderFactory.CreateGraphicsShaderPass(
            hlslCode,
            filePath,
            vertexShaderEntry,
            shaderModelVS,
            pixelShaderEntry,
            shaderModelPS,
            overrideRenderState,
            shaderPass))
        {
            MessageBoxW(NULL, L"Failed to create GraphicsShaderPass.", L"Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }

        // シェーダーが有効か確認
        if (!shaderPass.IsValid())
        {
            return nullptr;
        }

        if (!passTag.empty() && !passTags.emplace(passTag).second)
        {
            MessageBoxA(nullptr, "Shader pass tags must be unique.", "Shader Loader Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }

        shaderPass.SetTag(passTag);

        // Parserの宣言とReflectionのGPUレイアウトを対応付け、全Passでの一致も検証する。
        if (!ResolvePropertyLayout(properties, shaderPass))
        {
            MessageBoxW(NULL, L"Properties layout differs between shader passes.", L"Error", MB_OK | MB_ICONERROR);
            return nullptr;
        }

        shaderPasses.push_back(std::move(shaderPass));
    }

    // GraphicsShaderの生成
    GraphicsShader* shader = new GraphicsShader(
        shaderID,
        shaderName,
        filePath,
        std::move(shaderPasses),
        std::move(properties));

    return shader;
}

