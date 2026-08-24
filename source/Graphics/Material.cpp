#include "pch.h"
#include "Graphics/Material.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include "System/FileSystem.h"
#include "System/Logger.h"
#include "System/renderer.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <type_traits>

namespace
{
    MaterialID g_nextMaterialID = 1;

    UINT AlignConstantBufferSize(UINT size)
    {
        return (size + 15u) & ~15u;
    }
}

Material::Material(std::string name, const GraphicsShader& shader)
    : m_shader(&shader), m_id(g_nextMaterialID++), m_name(std::move(name))
{
    // Resource値はShaderPropertyIDを添字として保持する。
    const std::vector<ShaderPropertyDesc>& shaderProperties = shader.GetProperties();
    m_resourceValues.resize(shaderProperties.size());
    m_resourcePaths.resize(shaderProperties.size());
    m_resourceErrors.resize(shaderProperties.size());

    // Reflectionで確定した各Propertyの末尾から、b3に必要なバイト列のサイズを求める。
    UINT propertiesSize = 0;
    for (const ShaderPropertyDesc& property : shaderProperties)
    {
        if (property.storage == MaterialPropertyStorage::ConstantBuffer && property.offset != INVALID_PROPERTY_OFFSET)
            propertiesSize = std::max(propertiesSize, property.offset + property.size);
    }

    m_propertyData.resize(AlignConstantBufferSize(propertiesSize));
    CreatePropertiesBuffer();

    // 定数値とResource値を、それぞれShader宣言時の既定値で初期化する。
    for (const ShaderPropertyDesc& property : shaderProperties)
        SetDefaultValue(property);
}

const GraphicsShader& Material::GetShader() const
{
    return *m_shader;
}

const GraphicsShaderPass& Material::GetShaderPass(size_t passIndex) const
{
    return m_shader->GetShaderPasses().at(passIndex);
}

MaterialPropertyID Material::GetPropertyID(ShaderPropertyID shaderPropertyID) const
{
    // Shaderが宣言したPropertyだけを、Material固有の64bit IDへ変換する。
    return m_shader->GetProperty(shaderPropertyID)
        ? MakeMaterialPropertyID(m_id, shaderPropertyID)
        : MaterialPropertyID{};
}

MaterialPropertyID Material::GetPropertyID(std::string_view name) const
{
    // 名前の解決はShaderに集約し、Material側ではMaterial固有IDへ変換するだけにする。
    const ShaderPropertyDesc* property = m_shader->GetProperty(name);
    return property ? GetPropertyID(property->id) : MaterialPropertyID{};
}

const ShaderPropertyDesc* Material::FindShaderProperty(MaterialPropertyID id) const
{
    // 上位bitのMaterialIDが異なるPropertyは、このMaterialから操作できない。
    if (GetMaterialID(id) != m_id)
        return nullptr;

    return m_shader->GetProperty(GetShaderPropertyID(id));
}

const ConstantBufferInfo* Material::FindPropertiesBufferInfo(const GraphicsShaderPass& shaderPass) const
{
    for (const ConstantBufferInfo& candidate : shaderPass.GetConstantBufferInfo())
    {
        if (candidate.name == "Properties")
            return &candidate;
    }

    return nullptr;
}

void Material::CreatePropertiesBuffer()
{
    // Resourceだけを持つShaderには、b3用のGPUバッファを作成しない。
    if (m_propertyData.empty())
        return;

    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.ByteWidth = static_cast<UINT>(m_propertyData.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    Renderer::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &m_propertiesBuffer);
}

bool Material::SetConstantBufferValue(MaterialPropertyID id, PropertyType type, const void* value, size_t valueSize)
{
    // 共有FallbackはRenderer固有値で上書きせず、常にエラー表示用のマゼンタを維持する。
    if (!m_isValid)
    {
        return true;
    }

    // ID・Storage・型を検証して、Resourceや別MaterialのPropertyへの書き込みを防ぐ。
    const ShaderPropertyDesc* property = FindShaderProperty(id);
    if (!property || property->storage != MaterialPropertyStorage::ConstantBuffer || property->type != type)
        return false;

    if (property->offset == INVALID_PROPERTY_OFFSET || property->offset + property->size > m_propertyData.size())
        return false;

    // HLSLの行列は列ごとに16バイト境界へ配置されるため、通常のmemcpyでは格納しない。
    if (type == PropertyType::Matrix3x3)
    {
        const Matrix3x3& matrix = *static_cast<const Matrix3x3*>(value);
        for (UINT column = 0; column < 3; ++column)
        {
            const float packedColumn[3]
            {
                matrix.m[0][column],
                matrix.m[1][column],
                matrix.m[2][column],
            };
            std::memcpy(m_propertyData.data() + property->offset + (column * 16), packedColumn, sizeof(packedColumn));
        }
        return true;
    }

    if (type == PropertyType::Matrix4x4)
    {
        const Matrix4x4& matrix = *static_cast<const Matrix4x4*>(value);
        for (UINT column = 0; column < 4; ++column)
        {
            const float packedColumn[4]
            {
                matrix.m[0][column],
                matrix.m[1][column],
                matrix.m[2][column],
                matrix.m[3][column],
            };
            std::memcpy(m_propertyData.data() + property->offset + (column * 16), packedColumn, sizeof(packedColumn));
        }
        return true;
    }

    // スカラー・ベクトル・ColorはReflectionが返したoffsetへ直接格納できる。
    if (valueSize > property->size)
        return false;

    std::memcpy(m_propertyData.data() + property->offset, value, valueSize);
    return true;
}

bool Material::GetConstantBufferValue(MaterialPropertyID id, PropertyType type, void* outValue, size_t valueSize) const
{
    // Setterと同じ検証を行い、異なる型のバイト列を復元しない。
    const ShaderPropertyDesc* property = FindShaderProperty(id);
    if (!property || property->storage != MaterialPropertyStorage::ConstantBuffer || property->type != type)
        return false;

    if (property->offset == INVALID_PROPERTY_OFFSET || property->offset + property->size > m_propertyData.size())
        return false;

    // HLSL用に列単位で格納した行列を、CPU側の行列形式へ戻す。
    if (type == PropertyType::Matrix3x3)
    {
        Matrix3x3& matrix = *static_cast<Matrix3x3*>(outValue);
        for (UINT column = 0; column < 3; ++column)
        {
            float packedColumn[3]{};
            std::memcpy(packedColumn, m_propertyData.data() + property->offset + (column * 16), sizeof(packedColumn));

            matrix.m[0][column] = packedColumn[0];
            matrix.m[1][column] = packedColumn[1];
            matrix.m[2][column] = packedColumn[2];
        }
        return true;
    }

    if (type == PropertyType::Matrix4x4)
    {
        Matrix4x4& matrix = *static_cast<Matrix4x4*>(outValue);
        for (UINT column = 0; column < 4; ++column)
        {
            float packedColumn[4]{};
            std::memcpy(packedColumn, m_propertyData.data() + property->offset + (column * 16), sizeof(packedColumn));

            matrix.m[0][column] = packedColumn[0];
            matrix.m[1][column] = packedColumn[1];
            matrix.m[2][column] = packedColumn[2];
            matrix.m[3][column] = packedColumn[3];
        }
        return true;
    }

    // 行列以外は格納時と同じサイズで読み出す。
    if (valueSize > property->size)
        return false;

    std::memcpy(outValue, m_propertyData.data() + property->offset, valueSize);
    return true;
}

void Material::ShowPropertyError(const char* operation, MaterialPropertyID id) const
{
    // 型不一致や無効IDを呼び出し側で見逃さないよう、値の返却前に理由を通知する。
    const std::string message = "Material property operation failed.\nOperation: " + std::string(operation) +
        "\nMaterialPropertyID: " + std::to_string(id);
    MessageBoxA(nullptr, message.c_str(), "Material Property Error", MB_OK | MB_ICONERROR);
}

void Material::SetDefaultValue(const ShaderPropertyDesc& property)
{
    // 未使用の定数Propertyはコンパイラに除去されるため、GPUバイト列へは書き込まない。
    if (property.storage == MaterialPropertyStorage::ConstantBuffer && property.offset == INVALID_PROPERTY_OFFSET)
        return;

    const MaterialPropertyID id = GetPropertyID(property.id);
    const PropertyValue& value = property.defaultValue;

    // Parserが作った既定値を型付きSetterへ渡し、通常の設定経路と同じ検証・パックを利用する。
    if (const float* propertyValue = std::get_if<float>(&value)) SetFloat(id, *propertyValue);
    else if (const int* propertyValue = std::get_if<int>(&value)) SetInt(id, *propertyValue);
    else if (const bool* propertyValue = std::get_if<bool>(&value)) SetBool(id, *propertyValue);
    else if (const Vector2* propertyValue = std::get_if<Vector2>(&value)) SetVector2(id, *propertyValue);
    else if (const Vector3* propertyValue = std::get_if<Vector3>(&value)) SetVector3(id, *propertyValue);
    else if (const Vector4* propertyValue = std::get_if<Vector4>(&value)) SetVector4(id, *propertyValue);
    else if (const Matrix3x3* propertyValue = std::get_if<Matrix3x3>(&value)) SetMatrix3x3(id, *propertyValue);
    else if (const Matrix4x4* propertyValue = std::get_if<Matrix4x4>(&value)) SetMatrix4x4(id, *propertyValue);
    else if (const Color* propertyValue = std::get_if<Color>(&value)) SetColor(id, *propertyValue);
    else if (const TextureHandle* propertyValue = std::get_if<TextureHandle>(&value)) SetTexture(id, *propertyValue);
}

float Material::SetFloat(MaterialPropertyID id, float value)
{
    if (!SetConstantBufferValue(id, PropertyType::Float, &value, sizeof(value)))
    {
        ShowPropertyError("SetFloat", id);
        return 0.0f;
    }

    return value;
}

float Material::SetFloat(std::string_view name, float value)
{
    return SetFloat(GetPropertyID(name), value);
}

int Material::SetInt(MaterialPropertyID id, int value)
{
    if (!SetConstantBufferValue(id, PropertyType::Int, &value, sizeof(value)))
    {
        ShowPropertyError("SetInt", id);
        return 0;
    }

    return value;
}

int Material::SetInt(std::string_view name, int value)
{
    return SetInt(GetPropertyID(name), value);
}

bool Material::SetBool(MaterialPropertyID id, bool value)
{
    const UINT packedValue = value ? 1u : 0u;
    if (!SetConstantBufferValue(id, PropertyType::Bool, &packedValue, sizeof(packedValue)))
    {
        ShowPropertyError("SetBool", id);
        return false;
    }

    return value;
}

bool Material::SetBool(std::string_view name, bool value)
{
    return SetBool(GetPropertyID(name), value);
}

Vector2 Material::SetVector2(MaterialPropertyID id, const Vector2& value)
{
    if (!SetConstantBufferValue(id, PropertyType::Vector2, &value, sizeof(value)))
    {
        ShowPropertyError("SetVector2", id);
        return Vector2::Zero();
    }

    return value;
}

Vector2 Material::SetVector2(std::string_view name, const Vector2& value)
{
    return SetVector2(GetPropertyID(name), value);
}

Vector3 Material::SetVector3(MaterialPropertyID id, const Vector3& value)
{
    if (!SetConstantBufferValue(id, PropertyType::Vector3, &value, sizeof(value)))
    {
        ShowPropertyError("SetVector3", id);
        return Vector3::Zero();
    }

    return value;
}

Vector3 Material::SetVector3(std::string_view name, const Vector3& value)
{
    return SetVector3(GetPropertyID(name), value);
}

Vector4 Material::SetVector4(MaterialPropertyID id, const Vector4& value)
{
    if (!SetConstantBufferValue(id, PropertyType::Vector4, &value, sizeof(value)))
    {
        ShowPropertyError("SetVector4", id);
        return Vector4::Zero();
    }

    return value;
}

Vector4 Material::SetVector4(std::string_view name, const Vector4& value)
{
    return SetVector4(GetPropertyID(name), value);
}

Matrix3x3 Material::SetMatrix3x3(MaterialPropertyID id, const Matrix3x3& value)
{
    if (!SetConstantBufferValue(id, PropertyType::Matrix3x3, &value, sizeof(value)))
    {
        ShowPropertyError("SetMatrix3x3", id);
        return Matrix3x3{};
    }

    return value;
}

Matrix3x3 Material::SetMatrix3x3(std::string_view name, const Matrix3x3& value)
{
    return SetMatrix3x3(GetPropertyID(name), value);
}

Matrix4x4 Material::SetMatrix4x4(MaterialPropertyID id, const Matrix4x4& value)
{
    if (!SetConstantBufferValue(id, PropertyType::Matrix4x4, &value, sizeof(value)))
    {
        ShowPropertyError("SetMatrix4x4", id);
        return Matrix4x4{};
    }

    return value;
}

Matrix4x4 Material::SetMatrix4x4(std::string_view name, const Matrix4x4& value)
{
    return SetMatrix4x4(GetPropertyID(name), value);
}

Color Material::SetColor(MaterialPropertyID id, const Color& value)
{
    if (!SetConstantBufferValue(id, PropertyType::Color, &value, sizeof(value)))
    {
        ShowPropertyError("SetColor", id);
        return Color{};
    }

    return value;
}

Color Material::SetColor(std::string_view name, const Color& value)
{
    return SetColor(GetPropertyID(name), value);
}

TextureHandle Material::SetTexture(MaterialPropertyID id, TextureHandle texture)
{
    if (!m_isValid)
    {
        return texture;
    }

    // Textureは定数バッファに詰めず、ShaderPropertyIDで対応付けたResource値として保持する。
    const ShaderPropertyDesc* property = FindShaderProperty(id);
    if (!property || property->storage != MaterialPropertyStorage::Resource || property->type != PropertyType::Texture2D)
    {
        ShowPropertyError("SetTexture", id);
        return TextureHandle{};
    }

    m_resourceValues[property->id] = texture;
    const std::optional<std::string> sourcePath = TextureManager::GetSourcePath(texture);
    m_resourcePaths[property->id] = sourcePath.value_or(std::string{});
    m_resourceErrors[property->id] = false;
    return texture;
}

TextureHandle Material::SetTexture(std::string_view name, TextureHandle texture)
{
    return SetTexture(GetPropertyID(name), texture);
}

bool Material::SetTexturePath(MaterialPropertyID id, std::string path)
{
    if (!m_isValid)
    {
        return false;
    }

    const ShaderPropertyDesc* property = FindShaderProperty(id);
    if (!property || property->storage != MaterialPropertyStorage::Resource || property->type != PropertyType::Texture2D)
    {
        ShowPropertyError("SetTexturePath", id);
        return false;
    }

    TextureHandle texture;
    bool hasError = false;
    if (!path.empty())
    {
        texture = TextureManager::LoadTexture(path);
        hasError = !texture.IsValid();
        if (hasError)
        {
            Logger::Error("Material Textureを読み込めません。\nTexture: " + path);
        }
    }

    m_resourceValues[property->id] = texture;
    m_resourcePaths[property->id] = std::move(path);
    m_resourceErrors[property->id] = hasError;
    return !hasError;
}

bool Material::SetTexturePath(std::string_view name, std::string path)
{
    return SetTexturePath(GetPropertyID(name), std::move(path));
}

float Material::GetFloat(MaterialPropertyID id) const
{
    // Getterはプロパティの正であるバイト列から値を復元する。
    float value{};
    if (!GetConstantBufferValue(id, PropertyType::Float, &value, sizeof(value)))
        ShowPropertyError("GetFloat", id);

    return value;
}

float Material::GetFloat(std::string_view name) const
{
    return GetFloat(GetPropertyID(name));
}

int Material::GetInt(MaterialPropertyID id) const
{
    int value{};
    if (!GetConstantBufferValue(id, PropertyType::Int, &value, sizeof(value)))
        ShowPropertyError("GetInt", id);

    return value;
}

int Material::GetInt(std::string_view name) const
{
    return GetInt(GetPropertyID(name));
}

bool Material::GetBool(MaterialPropertyID id) const
{
    UINT packedValue{};
    if (!GetConstantBufferValue(id, PropertyType::Bool, &packedValue, sizeof(packedValue)))
    {
        ShowPropertyError("GetBool", id);
        return false;
    }

    return packedValue != 0;
}

bool Material::GetBool(std::string_view name) const
{
    return GetBool(GetPropertyID(name));
}

Vector2 Material::GetVector2(MaterialPropertyID id) const
{
    Vector2 value{};
    if (!GetConstantBufferValue(id, PropertyType::Vector2, &value, sizeof(value)))
        ShowPropertyError("GetVector2", id);

    return value;
}

Vector2 Material::GetVector2(std::string_view name) const
{
    return GetVector2(GetPropertyID(name));
}

Vector3 Material::GetVector3(MaterialPropertyID id) const
{
    Vector3 value{};
    if (!GetConstantBufferValue(id, PropertyType::Vector3, &value, sizeof(value)))
        ShowPropertyError("GetVector3", id);

    return value;
}

Vector3 Material::GetVector3(std::string_view name) const
{
    return GetVector3(GetPropertyID(name));
}

Vector4 Material::GetVector4(MaterialPropertyID id) const
{
    Vector4 value{};
    if (!GetConstantBufferValue(id, PropertyType::Vector4, &value, sizeof(value)))
        ShowPropertyError("GetVector4", id);

    return value;
}

Vector4 Material::GetVector4(std::string_view name) const
{
    return GetVector4(GetPropertyID(name));
}

Matrix3x3 Material::GetMatrix3x3(MaterialPropertyID id) const
{
    Matrix3x3 value{};
    if (!GetConstantBufferValue(id, PropertyType::Matrix3x3, &value, sizeof(value)))
        ShowPropertyError("GetMatrix3x3", id);

    return value;
}

Matrix3x3 Material::GetMatrix3x3(std::string_view name) const
{
    return GetMatrix3x3(GetPropertyID(name));
}

Matrix4x4 Material::GetMatrix4x4(MaterialPropertyID id) const
{
    Matrix4x4 value{};
    if (!GetConstantBufferValue(id, PropertyType::Matrix4x4, &value, sizeof(value)))
        ShowPropertyError("GetMatrix4x4", id);

    return value;
}

Matrix4x4 Material::GetMatrix4x4(std::string_view name) const
{
    return GetMatrix4x4(GetPropertyID(name));
}

Color Material::GetColor(MaterialPropertyID id) const
{
    Color value{};
    if (!GetConstantBufferValue(id, PropertyType::Color, &value, sizeof(value)))
        ShowPropertyError("GetColor", id);

    return value;
}

Color Material::GetColor(std::string_view name) const
{
    return GetColor(GetPropertyID(name));
}

TextureHandle Material::GetTexture(MaterialPropertyID id) const
{
    // TextureはGPUバイト列ではなく、Resource値の配列から返す。
    const ShaderPropertyDesc* property = FindShaderProperty(id);
    if (!property || property->storage != MaterialPropertyStorage::Resource || property->type != PropertyType::Texture2D)
    {
        ShowPropertyError("GetTexture", id);
        return TextureHandle{};
    }

    return m_resourceValues[property->id];
}

TextureHandle Material::GetTexture(std::string_view name) const
{
    return GetTexture(GetPropertyID(name));
}

const std::string& Material::GetTexturePath(MaterialPropertyID id) const
{
    static const std::string emptyPath;
    const ShaderPropertyDesc* property = FindShaderProperty(id);
    if (!property || property->storage != MaterialPropertyStorage::Resource || property->id >= m_resourcePaths.size())
    {
        return emptyPath;
    }
    return m_resourcePaths[property->id];
}

bool Material::HasTextureError(MaterialPropertyID id) const
{
    const ShaderPropertyDesc* property = FindShaderProperty(id);
    return property &&
        property->storage == MaterialPropertyStorage::Resource &&
        property->id < m_resourceErrors.size() &&
        m_resourceErrors[property->id];
}

bool Material::GetPropertyValue(const ShaderPropertyDesc& property, PropertyValue& value) const
{
    value = property.defaultValue;
    if (property.storage == MaterialPropertyStorage::ConstantBuffer && property.offset == INVALID_PROPERTY_OFFSET)
    {
        return true;
    }

    const MaterialPropertyID id = GetPropertyID(property.id);
    bool succeeded = true;
    std::visit([this, id, &property, &succeeded](auto& typedValue)
    {
        using T = std::decay_t<decltype(typedValue)>;
        if constexpr (std::is_same_v<T, bool>)
        {
            UINT packedValue{};
            succeeded = GetConstantBufferValue(id, property.type, &packedValue, sizeof(packedValue));
            typedValue = packedValue != 0;
        }
        else if constexpr (std::is_same_v<T, TextureHandle>)
        {
            typedValue = GetTexture(id);
        }
        else
        {
            succeeded = GetConstantBufferValue(id, property.type, &typedValue, sizeof(typedValue));
        }
    }, value);
    return succeeded;
}

bool Material::SetPropertyValue(const ShaderPropertyDesc& property, const PropertyValue& value)
{
    if (value.index() != property.defaultValue.index())
    {
        return false;
    }

    if (property.storage == MaterialPropertyStorage::ConstantBuffer && property.offset == INVALID_PROPERTY_OFFSET)
    {
        return true;
    }

    const MaterialPropertyID id = GetPropertyID(property.id);
    bool succeeded = true;
    std::visit([this, id, &property, &succeeded](const auto& typedValue)
    {
        using T = std::decay_t<decltype(typedValue)>;
        if constexpr (std::is_same_v<T, bool>)
        {
            const UINT packedValue = typedValue ? 1u : 0u;
            succeeded = SetConstantBufferValue(id, property.type, &packedValue, sizeof(packedValue));
        }
        else if constexpr (std::is_same_v<T, TextureHandle>)
        {
            SetTexture(id, typedValue);
        }
        else
        {
            succeeded = SetConstantBufferValue(id, property.type, &typedValue, sizeof(typedValue));
        }
    }, value);
    return succeeded;
}

void Material::BindProperties(const GraphicsShaderPass& shaderPass)
{
    // Propertiesを参照しないPassでは、b3への更新・Bindは不要である。
    if (!m_propertiesBuffer)
        return;

    const ConstantBufferInfo* propertiesInfo = FindPropertiesBufferInfo(shaderPass);
    if (!propertiesInfo)
        return;

    // CPU側の最新バイト列をGPUバッファへ転送し、Reflectionで確定した各ステージへBindする。
    ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();
    deviceContext->UpdateSubresource(m_propertiesBuffer.Get(), 0, nullptr, m_propertyData.data(), 0, 0);

    ID3D11Buffer* propertiesBuffer = m_propertiesBuffer.Get();
    if (propertiesInfo->vsSlot >= 0)
        deviceContext->VSSetConstantBuffers(propertiesInfo->vsSlot, 1, &propertiesBuffer);

    if (propertiesInfo->psSlot >= 0)
        deviceContext->PSSetConstantBuffers(propertiesInfo->psSlot, 1, &propertiesBuffer);
}

void Material::Bind(size_t passIndex)
{
    const GraphicsShaderPass& shaderPass = GetShaderPass(passIndex);
    ID3D11DeviceContext* deviceContext = Renderer::GetDeviceContext();

    // Passが決めたShader・頂点入力・RenderStateを先に反映する。
    shaderPass.Bind();

    // Propertiesの定数値を先にb3へ反映する。
    BindProperties(shaderPass);

    // Resource PropertyはPassごとのReflection結果に従って各シェーダーステージへ設定する。
    for (const ShaderTextureInfo& binding : shaderPass.GetTextureInfo())
    {
        const ShaderPropertyDesc* property = m_shader->GetProperty(binding.name);
        if (!property || property->storage != MaterialPropertyStorage::Resource)
            continue;

        const TextureHandle texture = m_resourceValues[property->id];
        ID3D11ShaderResourceView* resource = TextureManager::GetShaderResourceView(texture);

        if (binding.vsSlot >= 0)
            deviceContext->VSSetShaderResources(binding.vsSlot, 1, &resource);

        if (binding.psSlot >= 0)
            deviceContext->PSSetShaderResources(binding.psSlot, 1, &resource);
    }
}

bool Material::Bind(std::string_view passTag)
{
    const std::optional<size_t> passIndex = m_shader->GetShaderPassIndex(passTag);
    if (!passIndex)
        return false;

    Bind(*passIndex);
    return true;
}

void Material::DrawTexturePropertyImGui(const ShaderPropertyDesc& property, MaterialPropertyID id)
{
    const char* label = property.displayName.empty() ? property.name.c_str() : property.displayName.c_str();
    const std::string& path = GetTexturePath(id);
    if (HasTextureError(id))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "%s: Error", label);
    }
    else
    {
        ImGui::Text("%s: %s", label, path.empty() ? "None" : path.c_str());
    }

    if (ImGui::Button("Select"))
    {
        static constexpr std::array<std::string_view, 9> textureExtensions
        {
            ".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff", ".gif", ".wdp", ".jxr",
        };
        const std::optional<std::filesystem::path> selectedPath =
            FileSystem::OpenFileDialog("Assets", textureExtensions);
        if (selectedPath)
        {
            if (!FileSystem::IsUnderDirectory(*selectedPath, "Assets"))
            {
                Logger::Error("TextureはAssetsフォルダ配下に配置してください。");
            }
            else if (const std::optional<std::filesystem::path> assetPath =
                FileSystem::ToProjectRelative(*selectedPath))
            {
                SetTexturePath(id, assetPath->generic_string());
            }
        }
    }

    if (!path.empty())
    {
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
        {
            SetTexturePath(id, {});
        }
    }
}

void Material::DrawInspectorImGui()
{
    if (!m_isValid)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "Status: Error");
    }

    for (const ShaderPropertyDesc& property : m_shader->GetProperties())
    {
        const char* label = property.displayName.empty() ? property.name.c_str() : property.displayName.c_str();
        const MaterialPropertyID id = GetPropertyID(property.id);

        ImGui::PushID(static_cast<int>(property.id));
        if (property.storage == MaterialPropertyStorage::ConstantBuffer && property.offset == INVALID_PROPERTY_OFFSET)
        {
            ImGui::Text("%s: シェーダーで未使用", label);
        }
        else if (property.type == ValueType::Float)
        {
            float value = GetFloat(id);
            if (ImGui::DragFloat(label, &value, 0.01f)) SetFloat(id, value);
        }
        else if (property.type == ValueType::Int)
        {
            int value = GetInt(id);
            if (ImGui::DragInt(label, &value)) SetInt(id, value);
        }
        else if (property.type == ValueType::Bool)
        {
            bool value = GetBool(id);
            if (ImGui::Checkbox(label, &value)) SetBool(id, value);
        }
        else if (property.type == ValueType::Color)
        {
            Color value = GetColor(id);
            if (ImGui::ColorEdit4(label, &value.r)) SetColor(id, value);
        }
        else if (property.type == ValueType::Vector2)
        {
            Vector2 value = GetVector2(id);
            if (ImGui::DragFloat2(label, &value.x, 0.01f)) SetVector2(id, value);
        }
        else if (property.type == ValueType::Vector3)
        {
            Vector3 value = GetVector3(id);
            if (ImGui::DragFloat3(label, &value.x, 0.01f)) SetVector3(id, value);
        }
        else if (property.type == ValueType::Vector4)
        {
            Vector4 value = GetVector4(id);
            if (ImGui::DragFloat4(label, &value.x, 0.01f)) SetVector4(id, value);
        }
        else if (property.type == ValueType::Matrix3x3)
        {
            Matrix3x3 value = GetMatrix3x3(id);
            bool changed = false;

            ImGui::TextUnformatted(label);
            ImGui::Indent();
            for (int row = 0; row < 3; ++row)
            {
                ImGui::PushID(row);
                changed |= ImGui::DragFloat3("##Row", value.m[row], 0.01f);
                ImGui::PopID();
            }
            ImGui::Unindent();

            if (changed) SetMatrix3x3(id, value);
        }
        else if (property.type == ValueType::Matrix4x4)
        {
            Matrix4x4 value = GetMatrix4x4(id);
            bool changed = false;

            ImGui::TextUnformatted(label);
            ImGui::Indent();
            for (int row = 0; row < 4; ++row)
            {
                ImGui::PushID(row);
                changed |= ImGui::DragFloat4("##Row", value.m[row], 0.01f);
                ImGui::PopID();
            }
            ImGui::Unindent();

            if (changed) SetMatrix4x4(id, value);
        }
        else if (property.type == ValueType::Texture2D)
        {
            DrawTexturePropertyImGui(property, id);
        }
        else
        {
            ImGui::Text("%s: 未対応", label);
        }
        ImGui::PopID();
    }
}
