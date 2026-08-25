#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>

class GameObject;
class Material;
class RendererBase;

// 選択中GameObjectの詳細とComponentごとの項目を表示する。
class InspectorWindow final
{
    bool m_showLocalTransform = false;
    std::unordered_map<const Material*, std::array<char, 256>> m_materialNameBuffers;

    void DrawMaterialInspector(RendererBase& renderer, uint32_t slot);
    void DrawMaterialName(Material& material);
    static void CreateMaterial(RendererBase& renderer, uint32_t slot);
    static void SelectMaterial(RendererBase& renderer, uint32_t slot);
    static std::optional<std::filesystem::path> ToAssetRelativePath(
        const std::filesystem::path& selectedPath,
        const std::filesystem::path& assetDirectory,
        const wchar_t* extension,
        const char* assetName);

public:
    void DrawImGui(GameObject* selectedGameObject);
};
