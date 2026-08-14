#pragma once

#include "Graphics/GraphicInc.h"
#include "Graphics/Shader/ShaderInc.h"

#include <string_view>
#include <wrl/client.h>

class GraphicsShader;
class GraphicsShaderPass;

class Material
{
    const GraphicsShader* m_shader{};
    MaterialID m_id{};
    std::vector<uint8_t> m_propertyData{};
    std::vector<TextureHandle> m_resourceValues{};
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_propertiesBuffer{};

    const ShaderPropertyDesc* FindShaderProperty(MaterialPropertyID id) const;
    const ConstantBufferInfo* FindPropertiesBufferInfo(const GraphicsShaderPass& shaderPass) const;
    bool SetConstantBufferValue(MaterialPropertyID id, PropertyType type, const void* value, size_t valueSize);
    bool GetConstantBufferValue(MaterialPropertyID id, PropertyType type, void* outValue, size_t valueSize) const;
    void SetDefaultValue(const ShaderPropertyDesc& property);
    void ShowPropertyError(const char* operation, MaterialPropertyID id) const;
    void CreatePropertiesBuffer();
    void BindProperties(const GraphicsShaderPass& shaderPass);

public:
    explicit Material(const GraphicsShader& shader);

    MaterialID GetID() const { return m_id; }
    MaterialPropertyID GetPropertyID(ShaderPropertyID shaderPropertyID) const;
    MaterialPropertyID GetPropertyID(std::string_view name) const;

    const GraphicsShader& GetShader() const;
    const GraphicsShaderPass& GetShaderPass(size_t passIndex) const;

    float SetFloat(MaterialPropertyID id, float value);
    float SetFloat(std::string_view name, float value);
    int SetInt(MaterialPropertyID id, int value);
    int SetInt(std::string_view name, int value);
    bool SetBool(MaterialPropertyID id, bool value);
    bool SetBool(std::string_view name, bool value);
    Vector2 SetVector2(MaterialPropertyID id, const Vector2& value);
    Vector2 SetVector2(std::string_view name, const Vector2& value);
    Vector3 SetVector3(MaterialPropertyID id, const Vector3& value);
    Vector3 SetVector3(std::string_view name, const Vector3& value);
    Vector4 SetVector4(MaterialPropertyID id, const Vector4& value);
    Vector4 SetVector4(std::string_view name, const Vector4& value);
    Matrix3x3 SetMatrix3x3(MaterialPropertyID id, const Matrix3x3& value);
    Matrix3x3 SetMatrix3x3(std::string_view name, const Matrix3x3& value);
    Matrix4x4 SetMatrix4x4(MaterialPropertyID id, const Matrix4x4& value);
    Matrix4x4 SetMatrix4x4(std::string_view name, const Matrix4x4& value);
    Color SetColor(MaterialPropertyID id, const Color& value);
    Color SetColor(std::string_view name, const Color& value);
    TextureHandle SetTexture(MaterialPropertyID id, TextureHandle texture);
    TextureHandle SetTexture(std::string_view name, TextureHandle texture);

    float GetFloat(MaterialPropertyID id) const;
    float GetFloat(std::string_view name) const;
    int GetInt(MaterialPropertyID id) const;
    int GetInt(std::string_view name) const;
    bool GetBool(MaterialPropertyID id) const;
    bool GetBool(std::string_view name) const;
    Vector2 GetVector2(MaterialPropertyID id) const;
    Vector2 GetVector2(std::string_view name) const;
    Vector3 GetVector3(MaterialPropertyID id) const;
    Vector3 GetVector3(std::string_view name) const;
    Vector4 GetVector4(MaterialPropertyID id) const;
    Vector4 GetVector4(std::string_view name) const;
    Matrix3x3 GetMatrix3x3(MaterialPropertyID id) const;
    Matrix3x3 GetMatrix3x3(std::string_view name) const;
    Matrix4x4 GetMatrix4x4(MaterialPropertyID id) const;
    Matrix4x4 GetMatrix4x4(std::string_view name) const;
    Color GetColor(MaterialPropertyID id) const;
    Color GetColor(std::string_view name) const;
    TextureHandle GetTexture(MaterialPropertyID id) const;
    TextureHandle GetTexture(std::string_view name) const;

    void Bind(size_t passIndex);
    bool Bind(std::string_view passTag);
};
