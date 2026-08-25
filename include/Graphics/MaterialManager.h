#pragma once

#include "Graphics/GraphicInc.h"
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class GraphicsShader;
class Material;
struct ShaderPropertyDesc;
struct YAMLValueConversionResult;
struct YAMLValue;

// Materialの所有、ファイル入出力、参照切れ時のFallback選択を一元管理する。
class MaterialManager
{
    static std::unordered_map<MaterialID, std::unique_ptr<Material>> m_materials;
    static std::unordered_map<std::string, Material*> m_materialsByName;
    static std::unordered_map<std::string, Material*> m_materialsByPath;
    static std::unordered_map<ShaderID, Material*> m_defaultMaterials;
    static Material* m_fallbackMaterial;

    // Map検索に使う保存先を、区切り文字と相対要素を正規化して一意化する。
    static std::string NormalizePath(const std::filesystem::path& path);
    static Material* UseFallback(const std::string& path);

    template <typename T>
    static bool ReadYAMLValue(const YAMLValue& parent, std::string_view key, T& value);

    static YAMLValueConversionResult PropertyToYAMLValue(
        const Material& material,
        const ShaderPropertyDesc& property,
        YAMLValue& output);
    static bool PropertyFromYAMLValue(
        Material& material,
        const ShaderPropertyDesc& property,
        const YAMLValue& input);
    static bool ValidateDocument(
        const YAMLValue& document,
        std::string& name,
        std::string& shaderPath,
        const YAMLValue*& properties);

    static constexpr int MaterialFileVersion = 1;

public:
    static bool Init();
    static void Uninit();

    static Material* Create(std::string_view name, const GraphicsShader& shader);
    static bool Rename(Material& material, std::string_view name);
    static Material* Load(const std::filesystem::path& path);
    static bool Save(Material& material, const std::filesystem::path& path);
    static Material* GetDefault(const GraphicsShader& shader);
    static Material* GetFallback() { return m_fallbackMaterial; }
    static Material* Get(MaterialID id);
    static Material* Get(std::string_view name);
};
