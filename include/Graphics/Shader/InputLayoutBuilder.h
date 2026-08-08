/*=============================================================================

 File   : [InputLayoutBuilder.h]
 Desc   : 入力レイアウトの作成を行うクラスの宣言

 ------------------------------------------------------------------------------

 Date   : 2026/04/10
 Author : Yokoyama Haruki

===============================================================================*/

#ifndef _INPUT_LAYOUT_BUILDER_H_
#define _INPUT_LAYOUT_BUILDER_H_

#include "Graphics/GraphicInc.h"
#include "Graphics/Shader/ShaderInc.h"

#include <vector>
#include <d3d11.h>
#include <d3d11shader.h>

class GraphicsDevice;

class InputLayoutBuilder
{
    const GraphicsDevice& m_graphicsDevice;
    DXGI_FORMAT getFormat(const D3D11_SIGNATURE_PARAMETER_DESC& param);
    VertexStreamType getVertexAttribute(const D3D11_SIGNATURE_PARAMETER_DESC& param);

public:
    InputLayoutBuilder(const GraphicsDevice& graphicsDevice)
        : m_graphicsDevice(graphicsDevice)
    {
    }

    bool CreateInputLayout(
        ID3DBlob* shaderBlob,
        std::vector<InputSlotBinding>& outInputSlot,
        ID3D11InputLayout** outInputLayout);
};


#endif // _INPUT_LAYOUT_BUILDER_H_
