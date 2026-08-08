/*=============================================================================

File   : [ShaderManager.cpp]
Desc   : シェーダーの管理を行うクラスの実装

------------------------------------------------------------------------------

Date   : 2026/03/10
Author : Yokoyama Haruki

===============================================================================*/

#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/Shader/ShaderLoader.h"

GraphicsShader* ShaderManager::GetShaderByFilePath(const std::string& filePath) const
{
    for (const auto& shader : m_shaders)
    {
        if (shader->GetFilePath() == filePath)
        {
            return shader.get();
        }
    }
    return nullptr;
}

GraphicsShader* ShaderManager::GetShaderByID(ShaderID id) const
{
    for (const auto& shader : m_shaders)
    {
        if (shader->GetID() == id)
        {
            return shader.get();
        }
    }
    return nullptr;
}

GraphicsShader* ShaderManager::AddShader(GraphicsShader* shader)
{
    if (shader == nullptr || shader->IsValid() == false)
        return nullptr;

    m_shaders.emplace_back(std::unique_ptr<GraphicsShader>(shader));
    return shader;

}

GraphicsShader* ShaderManager::LoadShaderFromFile(const std::string& filePath)
{
    // 既に同じファイルパスのシェーダーが存在する場合は、それを返す
    GraphicsShader* existingShader = GetShaderByFilePath(filePath);
    if (existingShader != nullptr)
    {
        return existingShader;
    }

    // 新しいシェーダーをロードする
    ShaderID newShaderID = GenerateShaderID();
    GraphicsShader* newShader = m_shaderLoader.LoadShaderFromFile(filePath, newShaderID);
    if (newShader == nullptr)
    {
        return nullptr;
    }
    // シェーダーを管理リストに追加する
    AddShader(newShader);
    return newShader;
}


void ShaderManager::RemoveShaderByFilePath(const std::string& filePath)
{
    m_shaders.erase(
        std::remove_if(m_shaders.begin(), m_shaders.end(),
            [&filePath](const std::unique_ptr<GraphicsShader>& shader)
            {
                return shader->GetFilePath() == filePath;
            }),
        m_shaders.end());
}
