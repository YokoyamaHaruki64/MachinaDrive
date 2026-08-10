#include "pch.h"
#include "Graphics/Material.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include "System/renderer.h"

#include <cstddef>

namespace { MaterialID g_nextMaterialID = 1; }

Material::Material(const GraphicsShader& shader) : m_shader(&shader), m_id(g_nextMaterialID++)
{
    // Shaderが宣言した既定値をMaterialごとの編集可能な値として複製する。
    for (const ShaderPropertyDesc& shaderProperty : shader.GetProperties())
        m_properties.push_back({ shaderProperty.id, shaderProperty.defaultValue });
}

const GraphicsShader& Material::GetShader() const { return *m_shader; }

const GraphicsShaderPass& Material::GetShaderPass(size_t passIndex) const { return m_shader->GetShaderPasses().at(passIndex); }

const ShaderPropertyDesc* Material::FindShaderProperty(const std::string& name) const
{
    for (const ShaderPropertyDesc& candidate : m_shader->GetProperties())
    {
        if (candidate.name == name)
            return &candidate;
    }

    return nullptr;
}

const MaterialProperty* Material::FindPropertyValue(PropertyID id) const
{
    for (const MaterialProperty& candidate : m_properties)
    {
        if (candidate.id == id)
            return &candidate;
    }

    return nullptr;
}

bool Material::SetProperty(PropertyID id, const PropertyValue& value)
{
    // Property IDはShaderの宣言順で安定しているため、Material内の値だけを差し替える。
    for (MaterialProperty& materialProperty : m_properties)
    {
        if (materialProperty.id == id)
        {
            materialProperty.value = value;
            return true;
        }
    }

    return false;
}

bool Material::SetTexture(PropertyID id, TextureHandle texture)
{
    return SetProperty(id, texture);
}

void Material::Bind(size_t passIndex)
{
    const auto& shaderPass = GetShaderPass(passIndex);

    // Texture PropertyはPassごとに異なるステージ・レジスタへ設定される。
    const auto& passTextureInfo = shaderPass.GetTextureInfo();
    for (size_t bindingIndex = 0; bindingIndex < passTextureInfo.size(); ++bindingIndex)
    {
        const ShaderTextureInfo& binding = passTextureInfo[bindingIndex];
        const ShaderPropertyDesc* shaderProperty = FindShaderProperty(binding.name);
        if (!shaderProperty)
            continue;

        const MaterialProperty* materialProperty = FindPropertyValue(shaderProperty->id);
        const TextureHandle* texture = materialProperty ? std::get_if<TextureHandle>(&materialProperty->value) : nullptr;
        ID3D11ShaderResourceView* resource = TextureManager::GetShaderResourceView(texture ? *texture : TextureHandle{});
        if (binding.vsSlot >= 0) Renderer::GetDeviceContext()->VSSetShaderResources(binding.vsSlot, 1, &resource);
        if (binding.psSlot >= 0) Renderer::GetDeviceContext()->PSSetShaderResources(binding.psSlot, 1, &resource);
    }
}
