#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

#include "System/Editor/EditorWindow.h"
#include "System/Serialization/YAMLTypes.h"

class GameObject;
class Component;
class Material;
class PostProcess;
class RendererBase;

// 選択中GameObjectの詳細とComponentごとの項目を表示する。
class InspectorWindow final : public EditorWindow
{
    const std::vector<GameObject*>* m_selectedGameObjects = nullptr;
    bool m_showLocalTransform = false;
    GameObject* m_gameObjectNameTarget = nullptr;
    std::array<char, 256> m_gameObjectNameBuffer{};
    GameObject* m_gameObjectTagTarget = nullptr;
    std::array<char, 256> m_gameObjectTagBuffer{};
    std::unordered_map<const Material*, std::array<char, 256>> m_materialNameBuffers;
    std::unordered_map<Component*, YAMLValue> m_componentEditSnapshots;

    void DrawMaterialInspector(RendererBase& renderer, uint32_t slot);
    void DrawPostProcessMaterialInspector(PostProcess& postProcess);
    bool DrawMaterialName(Material& material);
    static bool SelectShader(Material& material);
    static void CreateMaterial(RendererBase& renderer, uint32_t slot);
    static void SelectMaterial(RendererBase& renderer, uint32_t slot);
    static std::optional<std::filesystem::path> ToAssetRelativePath(
        const std::filesystem::path& selectedPath,
        const std::filesystem::path& assetDirectory,
        const wchar_t* extension,
        const char* assetName);

public:
    void SetSelectedGameObjects(const std::vector<GameObject*>& selectedGameObjects) { m_selectedGameObjects = &selectedGameObjects; }
    void DrawImGui() override;
};
