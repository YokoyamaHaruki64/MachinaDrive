#pragma once

#include "Graphics/GraphicInc.h"
#include <memory>
#include <unordered_map>

class GraphicsShader;
class Material;

class MaterialManager
{
    static std::unordered_map<MaterialID, std::unique_ptr<Material>> m_materials;
    static std::unordered_map<ShaderID, Material*> m_defaultMaterials;

public:
    static void Init();
    static void Uninit();

    static Material* Create(const GraphicsShader& shader);
    static Material* GetDefault(const GraphicsShader& shader);
    static Material* Get(MaterialID id);
};
