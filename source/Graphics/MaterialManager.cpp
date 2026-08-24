#include "pch.h"

#include "Graphics/MaterialManager.h"

#include "Graphics/Material.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/Logger.h"
#include "System/YAMLArchive.h"
#include "System/YAMLCodec.h"

std::unordered_map<MaterialID, std::unique_ptr<Material>> MaterialManager::m_materials;
std::unordered_map<std::string, Material*> MaterialManager::m_materialsByName;
std::unordered_map<std::string, Material*> MaterialManager::m_materialsByPath;
std::unordered_map<ShaderID, Material*> MaterialManager::m_defaultMaterials;
Material* MaterialManager::m_fallbackMaterial{};

class MaterialManager::Implementation final
{
public:
    static constexpr int VERSION = 1;

    static Material* UseFallback(const std::string& path)
    {
        if (!path.empty() && MaterialManager::m_fallbackMaterial)
        {
            MaterialManager::m_materialsByPath.try_emplace(path, MaterialManager::m_fallbackMaterial);
        }
        return MaterialManager::m_fallbackMaterial;
    }

    template <typename T>
    static bool ReadValue(const YAMLValue& parent, std::string_view key, T& value)
    {
        const YAMLValue* node = parent.Find(key);
        return node && YAMLCodec<T>::Decode(*node, value, {}).success;
    }

    static YAMLCodecResult EncodeProperty(
        const Material& material,
        const ShaderPropertyDesc& property,
        YAMLValue& output)
    {
        if (property.type == PropertyType::Texture2D)
        {
            const MaterialPropertyID id = material.GetPropertyID(property.id);
            const std::string& path = material.GetTexturePath(id);
            if (!path.empty())
            {
                return YAMLCodec<std::string>::Encode(path, output, {});
            }

            output = YAMLValue::Null();
            if (material.GetTexture(id).IsValid())
            {
                return {
                    true,
                    YAMLMessageLevel::Warning,
                    "ファイルパスを持たないTextureをNoneとして保存します",
                };
            }
            return {};
        }

        PropertyValue value;
        if (!material.GetPropertyValue(property, value))
        {
            return { false };
        }

        YAMLCodecResult result{ false };
        std::visit([&output, &result](const auto& typedValue)
        {
            using T = std::decay_t<decltype(typedValue)>;
            if constexpr (!std::is_same_v<T, TextureHandle>)
            {
                result = YAMLCodec<T>::Encode(typedValue, output, {});
            }
        }, value);
        return result;
    }

    static bool DecodeProperty(
        Material& material,
        const ShaderPropertyDesc& property,
        const YAMLValue& input)
    {
        if (property.type == PropertyType::Texture2D)
        {
            if (input.type == YAMLValue::Type::Null)
            {
                material.SetTexturePath(material.GetPropertyID(property.id), {});
                return true;
            }

            std::string path;
            if (!YAMLCodec<std::string>::Decode(input, path, {}).success)
            {
                return false;
            }

            // Texture単体の参照切れはMaterial全体を失敗させず、内部でFallbackTextureへ解決する。
            material.SetTexturePath(material.GetPropertyID(property.id), std::move(path));
            return true;
        }

        PropertyValue value = property.defaultValue;
        YAMLCodecResult result{ false };
        std::visit([&input, &result](auto& typedValue)
        {
            using T = std::decay_t<decltype(typedValue)>;
            if constexpr (!std::is_same_v<T, TextureHandle>)
            {
                result = YAMLCodec<T>::Decode(input, typedValue, {});
            }
        }, value);
        return result.success && material.SetPropertyValue(property, value);
    }

    static bool ValidateDocument(
        const YAMLValue& document,
        std::string& name,
        std::string& shaderPath,
        const YAMLValue*& properties)
    {
        int version{};
        if (document.type != YAMLValue::Type::Map ||
            !ReadValue(document, "version", version) ||
            version != VERSION ||
            !ReadValue(document, "name", name) ||
            name.empty() ||
            !ReadValue(document, "shader", shaderPath) ||
            shaderPath.empty())
        {
            return false;
        }

        properties = document.Find("properties");
        return properties && properties->type == YAMLValue::Type::Map;
    }
};

std::string MaterialManager::NormalizePath(const std::filesystem::path& path)
{
    return path.lexically_normal().generic_string();
}

bool MaterialManager::Init()
{
    GraphicsShader* defaultShader = ShaderManager::GetDefaultShader();
    if (!defaultShader)
    {
        Logger::Error("Fallback Material用の既定Shaderがありません。");
        return false;
    }

    m_fallbackMaterial = Create("Fallback.Material", *defaultShader);
    if (!m_fallbackMaterial)
    {
        Logger::Error("Fallback Materialを生成できません。");
        return false;
    }
    m_fallbackMaterial->SetColor("_BaseColor", Color::Magenta());
    m_fallbackMaterial->SetColor("_Emission", Color::Magenta());
    m_fallbackMaterial->m_isValid = false;
    return true;
}

void MaterialManager::Uninit()
{
    // Materialが参照するShaderより先に、すべてのMaterialを破棄する。
    m_fallbackMaterial = nullptr;
    m_defaultMaterials.clear();
    m_materialsByPath.clear();
    m_materialsByName.clear();
    m_materials.clear();
}

Material* MaterialManager::Create(std::string_view name, const GraphicsShader& shader)
{
    if (name.empty())
    {
        Logger::Error("Material名が空です。");
        return nullptr;
    }

    auto material = std::make_unique<Material>(std::string(name), shader);
    Material* result = material.get();

    m_materials.emplace(result->GetID(), std::move(material));
    // 同名Materialは許容し、名前検索では最初に生成したものを返す。
    m_materialsByName.try_emplace(result->GetName(), result);
    return result;
}

Material* MaterialManager::Load(const std::filesystem::path& path)
{
    const std::string normalizedPath = NormalizePath(path);
    if (normalizedPath.empty())
    {
        Logger::Error("Materialパスが空です。");
        return Implementation::UseFallback(normalizedPath);
    }

    const auto cached = m_materialsByPath.find(normalizedPath);
    if (cached != m_materialsByPath.end())
    {
        return cached->second;
    }

    YAMLValue document;
    if (!YAMLArchive::LoadDocument(normalizedPath, document))
    {
        return Implementation::UseFallback(normalizedPath);
    }

    std::string name;
    std::string shaderPath;
    const YAMLValue* properties{};
    if (!Implementation::ValidateDocument(document, name, shaderPath, properties))
    {
        Logger::Error("Materialファイルの形式が不正です。\nMaterial: " + normalizedPath);
        return Implementation::UseFallback(normalizedPath);
    }

    GraphicsShader* shader = ShaderManager::LoadShaderFromFile(shaderPath);
    if (!shader)
    {
        Logger::Error("MaterialのShaderを解決できません。\nMaterial: " + normalizedPath + "\nShader: " + shaderPath);
        return Implementation::UseFallback(normalizedPath);
    }

    Material* material = Create(name, *shader);
    if (!material)
    {
        return Implementation::UseFallback(normalizedPath);
    }

    for (const ShaderPropertyDesc& property : shader->GetProperties())
    {
        const YAMLValue* value = properties->Find(property.name);
        if (value && !Implementation::DecodeProperty(*material, property, *value))
        {
            Logger::Error("Material Propertyの読み込みに失敗しました。\nMaterial: " + normalizedPath +
                "\nProperty: " + property.name);
        }
    }

    for (const auto& [propertyName, value] : properties->map)
    {
        if (!shader->GetProperty(propertyName))
        {
            Logger::Error("未登録のMaterial Propertyをスキップしました。\nMaterial: " + normalizedPath +
                "\nProperty: " + propertyName);
        }
    }

    material->m_filePath = normalizedPath;
    m_materialsByPath.emplace(normalizedPath, material);
    return material;
}

bool MaterialManager::Save(Material& material, const std::filesystem::path& path)
{
    const std::string normalizedPath = NormalizePath(path);
    if (normalizedPath.empty())
    {
        Logger::Error("Material保存先が空です。");
        return false;
    }
    if (!material.IsValid())
    {
        Logger::Error("Fallback Materialは保存できません。");
        return false;
    }

    const auto existing = m_materialsByPath.find(normalizedPath);
    if (existing != m_materialsByPath.end() &&
        existing->second != &material &&
        existing->second != m_fallbackMaterial)
    {
        Logger::Error("別のMaterialが同じ保存先を使用しています。\nMaterial: " + normalizedPath);
        return false;
    }

    YAMLValue document = YAMLValue::Map();
    YAMLValue version;
    YAMLCodec<int>::Encode(Implementation::VERSION, version, {});
    document.Add("version", std::move(version));
    document.Add("name", YAMLValue::Scalar(material.GetName(), true));
    document.Add("shader", YAMLValue::Scalar(material.GetShader().GetFilePath(), true));

    YAMLValue properties = YAMLValue::Map();
    for (const ShaderPropertyDesc& property : material.GetShader().GetProperties())
    {
        YAMLValue value;
        const YAMLCodecResult result = Implementation::EncodeProperty(material, property, value);
        if (!result.message.empty())
        {
            Logger::Warning(result.message + ": " + property.name);
        }
        if (!result.success)
        {
            Logger::Error("Material Propertyの保存に失敗しました。\nMaterial: " + normalizedPath +
                "\nProperty: " + property.name);
            return false;
        }
        properties.Add(property.name, std::move(value));
    }
    document.Add("properties", std::move(properties));

    if (!YAMLArchive::SaveDocument(normalizedPath, document))
    {
        return false;
    }

    if (!material.m_filePath.empty())
    {
        m_materialsByPath.erase(material.m_filePath);
    }
    material.m_filePath = normalizedPath;
    m_materialsByPath[normalizedPath] = &material;
    return true;
}

Material* MaterialManager::GetDefault(const GraphicsShader& shader)
{
    const auto existing = m_defaultMaterials.find(shader.GetID());
    if (existing != m_defaultMaterials.end())
    {
        return existing->second;
    }

    // Shaderが宣言する既定Property値だけを持つ、Shader単位の共有Materialを遅延生成する。
    const std::string materialName = "Default." + shader.GetShaderName();
    Material* material = Create(materialName, shader);
    if (!material)
    {
        return nullptr;
    }

    m_defaultMaterials.emplace(shader.GetID(), material);
    return material;
}

Material* MaterialManager::Get(MaterialID id)
{
    const auto material = m_materials.find(id);
    return material == m_materials.end() ? nullptr : material->second.get();
}

Material* MaterialManager::Get(std::string_view name)
{
    const auto material = m_materialsByName.find(std::string(name));
    return material == m_materialsByName.end() ? nullptr : material->second;
}
