/*=============================================================================

File   : [ShaderCompiler.cpp]
Desc   : シェーダーのコンパイルを行うクラスの実装

------------------------------------------------------------------------------

Date   : 2026/04/10
Author : Yokoyama Haruki

===============================================================================*/

#include "Graphics/Shader/ShaderCompiler.h"

#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

bool ShaderCompiler::CompileShader(
    const std::string& hlslCode,
    const std::string& entryPoint,
    const std::string& shaderModel,
    ID3DBlob** outShaderBlob)
{
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(
        hlslCode.data(),
        hlslCode.size(),
        nullptr, // ソース名はエラーメッセージに表示されるだけなので、今回は指定しない
        nullptr, // マクロ定義は今回は使用しない
        D3D_COMPILE_STANDARD_FILE_INCLUDE, // #includeディレクティブを処理するためのフラグ
        entryPoint.c_str(),
        shaderModel.c_str(),
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
        0,
        outShaderBlob,
        errorBlob.GetAddressOf() // エラーが発生した場合、エラーメッセージがerrorBlobに格納される
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            std::string errorMessage(static_cast<const char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize());
            MessageBoxA(NULL, errorMessage.c_str(), "GraphicsShaderPass Compilation Error", MB_OK | MB_ICONERROR);
        }
        else
        {
            MessageBoxW(NULL, L"Unknown error occurred during shader compilation.", L"Error", MB_OK | MB_ICONERROR);
        }
        return false;
    }

    return true;
}

