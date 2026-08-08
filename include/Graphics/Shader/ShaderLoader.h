/*=============================================================================
 File   : [ShaderLoader.h]
 Desc   : シェーダーの読み込みとコンパイルを行うクラスの宣言

 ------------------------------------------------------------------------------

 Date   : 2026/03/12
 Author : Yokoyama Haruki

===============================================================================*/

#ifndef _SHADER_LOADER_H_
#define _SHADER_LOADER_H_

#include <string>
#include <memory>

#include "Graphics/Shader/ShaderFactory.h"
#include "Graphics/GraphicInc.h"

class GraphicsDevice;
class ShaderFactory;
class GraphicsShader;

class ShaderLoader
{
private:

    ShaderFactory m_shaderFactory;

public:
    ShaderLoader() = delete;
    ShaderLoader(const GraphicsDevice& graphicsDevice)
        : m_shaderFactory(graphicsDevice)
    {
    }

    GraphicsShader* LoadShaderFromFile(const std::string& filePath, ShaderID shaderID);

    ~ShaderLoader() = default;
};

#endif // _SHADER_LOADER_H_
