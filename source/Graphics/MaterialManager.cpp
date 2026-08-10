#include "pch.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/Material.h"

std::unordered_map<MaterialID, std::unique_ptr<Material>> MaterialManager::m_materials;

void MaterialManager::Init()
{
    // 静的コンテナは初期化済みのため、ライフサイクルの開始位置だけを明示する。
}

void MaterialManager::Uninit()
{
    // Materialが参照するShaderより先に、すべてのMaterialを破棄する。
    m_materials.clear();
}

Material* MaterialManager::Create(const GraphicsShader& shader)
{
    auto material = std::make_unique<Material>(shader);
    Material* result = material.get();

    m_materials.emplace(result->GetID(), std::move(material));
    return result;
}

Material* MaterialManager::Get(MaterialID id)
{
    const auto material = m_materials.find(id);
    return material == m_materials.end() ? nullptr : material->second.get();
}
