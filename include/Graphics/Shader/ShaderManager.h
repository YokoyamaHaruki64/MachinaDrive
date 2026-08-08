/*=============================================================================

File   : [ShaderManager.h]
Desc   : シェーダーの管理を行うクラスの宣言

------------------------------------------------------------------------------

Date   : 2026/03/13
Author : Yokoyama Haruki

===============================================================================*/

#ifndef _SHADER_MANAGER_H_
#define _SHADER_MANAGER_H_

#include "Graphics/GraphicInc.h"

class GraphicsShader;
class ShaderLoader;
class GraphicsDevice;

class ShaderManager
{
private:
    std::vector<std::unique_ptr<GraphicsShader>> m_shaders{};
    ShaderID m_shaderIDs{ 0 };

    ShaderLoader& m_shaderLoader;
public:

    ShaderManager(ShaderLoader& shaderLoader)
        : m_shaderLoader(shaderLoader)
    {
    }

    GraphicsShader* AddShader(GraphicsShader* shader);
    GraphicsShader* LoadShaderFromFile(const std::string& filePath);

    ShaderID GenerateShaderID()
    {
        return m_shaderIDs++;
    }

    GraphicsShader* GetShaderByFilePath(const std::string& filePath) const;
    GraphicsShader* GetShaderByID(ShaderID id) const;
    void RemoveShaderByFilePath(const std::string& filePath);
};


#endif // _SHADER_MANAGER_H_
