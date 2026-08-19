#pragma once

#include "Graphics/GraphicInc.h"
#include <memory>
#include <string_view>
#include <unordered_map>

class GraphicsShader;
class Material;

class MaterialManager
{
    static std::unordered_map<MaterialID, std::unique_ptr<Material>> m_materials;
    static std::unordered_map<std::string, Material*> m_materialsByName;
    static std::unordered_map<ShaderID, Material*> m_defaultMaterials;

public:
    static void Init();
    static void Uninit();

    static Material* Create(std::string_view name, const GraphicsShader& shader);
    static Material* GetDefault(const GraphicsShader& shader);
    static Material* Get(MaterialID id);
    static Material* Get(std::string_view name);
};
