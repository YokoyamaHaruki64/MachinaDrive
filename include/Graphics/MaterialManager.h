#pragma once

#include "Graphics/GraphicInc.h"
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class GraphicsShader;
class Material;

class MaterialManager
{
    static std::unordered_map<MaterialID, std::unique_ptr<Material>> m_materials;
    static std::unordered_map<std::string, Material*> m_materialsByName;
    static std::unordered_map<std::string, Material*> m_materialsByPath;
    static std::unordered_map<ShaderID, Material*> m_defaultMaterials;
    static Material* m_fallbackMaterial;

    static std::string NormalizePath(const std::filesystem::path& path);

    class Implementation;

public:
    static bool Init();
    static void Uninit();

    static Material* Create(std::string_view name, const GraphicsShader& shader);
    static Material* Load(const std::filesystem::path& path);
    static bool Save(Material& material, const std::filesystem::path& path);
    static Material* GetDefault(const GraphicsShader& shader);
    static Material* GetFallback() { return m_fallbackMaterial; }
    static Material* Get(MaterialID id);
    static Material* Get(std::string_view name);
};
