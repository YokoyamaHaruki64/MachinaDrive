#include "pch.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/Material.h"
#include "Graphics/Shader/ShaderProgram.h"

std::unordered_map<MaterialID, std::unique_ptr<Material>> MaterialManager::m_materials;
std::unordered_map<ShaderID, Material*> MaterialManager::m_defaultMaterials;

void MaterialManager::Init()
{
    // 静的コンテナは初期化済みのため、ライフサイクルの開始位置だけを明示する。
}

void MaterialManager::Uninit()
{
    // Materialが参照するShaderより先に、すべてのMaterialを破棄する。
    m_defaultMaterials.clear();
    m_materials.clear();
}

Material* MaterialManager::Create(const GraphicsShader& shader)
{
    auto material = std::make_unique<Material>(shader);
    Material* result = material.get();

    m_materials.emplace(result->GetID(), std::move(material));
    return result;
}

Material* MaterialManager::GetDefault(const GraphicsShader& shader)
{
    const auto existing = m_defaultMaterials.find(shader.GetID());
    if (existing != m_defaultMaterials.end())
        return existing->second;

    // Shaderが宣言する既定Property値だけを持つ、Shader単位の共有Materialを遅延生成する。
    Material* material = Create(shader);
    m_defaultMaterials.emplace(shader.GetID(), material);
    return material;
}

Material* MaterialManager::Get(MaterialID id)
{
    const auto material = m_materials.find(id);
    return material == m_materials.end() ? nullptr : material->second.get();
}
