#include "pch.h"

#include "Graphics/MaterialManager.h"

#include "Graphics/Material.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "System/Diagnostics/Logger.h"
#include "System/Serialization/YAMLDocumentIO.h"
#include "System/Serialization/YAMLValueConverter.h"

std::unordered_map<MaterialID, std::unique_ptr<Material>> MaterialManager::m_materials;
std::unordered_map<std::string, Material*> MaterialManager::m_materialsByName;
std::unordered_map<std::string, Material*> MaterialManager::m_materialsByPath;
std::unordered_map<ShaderID, Material*> MaterialManager::m_defaultMaterials;
Material* MaterialManager::m_fallbackMaterial{};

Material* MaterialManager::UseFallback(const std::string& path)
{
    // 同じ失敗パスを毎回解析しないよう、Fallbackもパス単位でキャッシュする。
    if (!path.empty() && m_fallbackMaterial)
    {
        m_materialsByPath.try_emplace(path, m_fallbackMaterial);
    }
    return m_fallbackMaterial;
}

template <typename T>
bool MaterialManager::ReadYAMLValue(const YAMLValue& parent, std::string_view key, T& value)
{
    // Material文書のMap要素を取得し、型CodecでC++値へ変換する。
    const YAMLValue* node = parent.Find(key);
    return node && YAMLValueConverter<T>::FromYAMLValue(*node, value, {}).success;
}

YAMLValueConversionResult MaterialManager::PropertyToYAMLValue(
    const Material& material,
    const ShaderPropertyDesc& property,
    YAMLValue& output)
{
    if (property.type == PropertyType::Texture2D)
    {
        // GPUハンドルではなく、再ロード可能なアセットパスだけを永続化する。
        const MaterialPropertyID id = material.GetPropertyID(property.id);
        const std::string& path = material.GetTexturePath(id);
        if (!path.empty())
        {
            return YAMLValueConverter<std::string>::ToYAMLValue(path, output, {});
        }

        output = YAMLValue::Null();
        return {};
    }

    PropertyValue value;
    if (!material.GetPropertyValue(property, value))
    {
        return { false };
    }

    YAMLValueConversionResult result{ false };
    std::visit([&output, &result](const auto& typedValue)
    {
        using T = std::decay_t<decltype(typedValue)>;
        if constexpr (!std::is_same_v<T, TextureHandle>)
        {
            result = YAMLValueConverter<T>::ToYAMLValue(typedValue, output, {});
        }
    }, value);
    return result;
}

bool MaterialManager::PropertyFromYAMLValue(
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
        if (!YAMLValueConverter<std::string>::FromYAMLValue(input, path, {}).success)
        {
            return false;
        }

        // Texture単体の参照切れはMaterial全体を失敗させず、内部でFallbackTextureへ解決する。
        material.SetTexturePath(material.GetPropertyID(property.id), std::move(path));
        return true;
    }

    // Shader宣言の既定値を土台にして、YAMLの値だけを型安全に上書きする。
    PropertyValue value = property.defaultValue;
    YAMLValueConversionResult result{ false };
    std::visit([&input, &result](auto& typedValue)
    {
        using T = std::decay_t<decltype(typedValue)>;
        if constexpr (!std::is_same_v<T, TextureHandle>)
        {
            result = YAMLValueConverter<T>::FromYAMLValue(input, typedValue, {});
        }
    }, value);
    return result.success && material.SetPropertyValue(property, value);
}

bool MaterialManager::ValidateDocument(
    const YAMLValue& document,
    std::string& name,
    std::string& shaderPath,
    const YAMLValue*& properties)
{
    // Material生成前に、復元に必須のヘッダーとProperty Mapだけを検証する。
    int version{};
    if (document.type != YAMLValue::Type::Map ||
        !ReadYAMLValue(document, "version", version) ||
        version != MaterialFileVersion ||
        !ReadYAMLValue(document, "name", name) ||
        name.empty() ||
        !ReadYAMLValue(document, "shader", shaderPath) ||
        shaderPath.empty())
    {
        return false;
    }

    properties = document.Find("properties");
    return properties && properties->type == YAMLValue::Type::Map;
}

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

bool MaterialManager::Rename(Material& material, std::string_view name)
{
    if (name.empty())
    {
        Logger::Error("Material名は空にできません。");
        return false;
    }
    if (material.m_name == name)
    {
        return true;
    }

    material.m_name = name;

    // 同名Materialを許可しているため、先に生成されたものを名前検索の代表として再構築する。
    m_materialsByName.clear();
    for (const auto& entry : m_materials)
    {
        Material* ownedMaterial = entry.second.get();
        auto [nameIterator, inserted] = m_materialsByName.try_emplace(
            ownedMaterial->GetName(),
            ownedMaterial);
        if (!inserted && ownedMaterial->GetID() < nameIterator->second->GetID())
        {
            nameIterator->second = ownedMaterial;
        }
    }
    return true;
}

Material* MaterialManager::Load(const std::filesystem::path& path)
{
    // パス単位で共有し、同じMaterialファイルを複数回生成しない。
    const std::string normalizedPath = NormalizePath(path);
    if (normalizedPath.empty())
    {
        Logger::Error("Materialパスが空です。");
        return UseFallback(normalizedPath);
    }

    const auto cached = m_materialsByPath.find(normalizedPath);
    if (cached != m_materialsByPath.end())
    {
        return cached->second;
    }

    YAMLValue document;
    if (!YAMLDocumentIO::Load(normalizedPath, document))
    {
        return UseFallback(normalizedPath);
    }

    std::string name;
    std::string shaderPath;
    const YAMLValue* properties{};
    if (!ValidateDocument(document, name, shaderPath, properties))
    {
        Logger::Error("Materialファイルの形式が不正です。\nMaterial: " + normalizedPath);
        return UseFallback(normalizedPath);
    }

    GraphicsShader* shader = ShaderManager::LoadShaderFromFile(shaderPath);
    if (!shader)
    {
        Logger::Error("MaterialのShaderを解決できません。\nMaterial: " + normalizedPath + "\nShader: " + shaderPath);
        return UseFallback(normalizedPath);
    }

    Material* material = Create(name, *shader);
    if (!material)
    {
        return UseFallback(normalizedPath);
    }

    for (const ShaderPropertyDesc& property : shader->GetProperties())
    {
        const YAMLValue* value = properties->Find(property.name);
        if (value && !PropertyFromYAMLValue(*material, property, *value))
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
    // 保存成功後にだけパス検索Mapを更新し、失敗時は既存参照を維持する。
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
    YAMLValueConverter<int>::ToYAMLValue(MaterialFileVersion, version, {});
    document.Add("version", std::move(version));
    document.Add("name", YAMLValue::Scalar(material.GetName(), true));
    document.Add("shader", YAMLValue::Scalar(material.GetShader().GetFilePath(), true));

    YAMLValue properties = YAMLValue::Map();
    for (const ShaderPropertyDesc& property : material.GetShader().GetProperties())
    {
        YAMLValue value;
        const YAMLValueConversionResult result = PropertyToYAMLValue(material, property, value);
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

    if (!YAMLDocumentIO::Save(normalizedPath, document))
    {
        return false;
    }

    if (!material.m_filePath.empty())
    {
        m_materialsByPath.erase(material.m_filePath);
    }
    material.m_filePath = normalizedPath;
    m_materialsByPath[normalizedPath] = &material;
    material.m_isDirty = false;
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
