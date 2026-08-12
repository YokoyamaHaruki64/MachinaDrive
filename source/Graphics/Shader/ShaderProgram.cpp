#include "pch.h"
#include "Graphics/Shader/ShaderProgram.h"

const ShaderPropertyDesc* GraphicsShader::GetProperty(ShaderPropertyID id) const
{
    // Property IDはParserが宣言順に採番するため、配列の添字として解決できる。
    if (id >= m_properties.size())
        return nullptr;

    return &m_properties[id];
}

const ShaderPropertyDesc* GraphicsShader::GetProperty(std::string_view name) const
{
    // 名前指定APIでは、Shader生成時に作成した名前→IDテーブルを使って線形探索を避ける。
    const auto found = m_propertyIDsByName.find(std::string(name));
    return found == m_propertyIDsByName.end() ? nullptr : GetProperty(found->second);
}
