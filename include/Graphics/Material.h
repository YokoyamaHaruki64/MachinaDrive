#pragma once

#include <unordered_map>
#include <wrl/client.h>
#include "Graphics/GraphicInc.h"
#include "Graphics/Shader/ShaderInc.h"

class GraphicsShader;
class GraphicsShaderPass;

class Material
{
    const GraphicsShader* m_shader{};
    MaterialID m_id{};
    std::vector<MaterialProperty> m_properties{};
    std::unordered_map<PropertyID, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textures{};
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_propertiesBuffer{};
    UINT m_propertiesBufferSize{};

    const ShaderPropertyDesc* FindShaderProperty(const std::string& name) const;
    const MaterialProperty* FindPropertyValue(PropertyID id) const;

public:
    explicit Material(const GraphicsShader& shader);
    MaterialID GetID() const { return m_id; }
    const GraphicsShader& GetShader() const;
    const GraphicsShaderPass& GetShaderPass(size_t passIndex) const;
    bool SetProperty(PropertyID id, const PropertyValue& value);
    bool SetTexture(PropertyID id, ID3D11ShaderResourceView* texture);
    void Bind(size_t passIndex);
};
