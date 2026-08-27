#pragma once

#include <filesystem>
#include <optional>

#include "System/Editor/EditorWindow.h"

class GameObject;

class HierarchyWindow final : public EditorWindow
{
    GameObject** m_selectedGameObject = nullptr;

    void DrawGameObjectNode(GameObject& gameObject, GameObject*& selectedGameObject);
    void DrawRootDropTarget(GameObject*& selectedGameObject);
    static std::optional<std::filesystem::path> ToAssetRelativePath(const std::filesystem::path& selectedPath);

public:
    void SetSelectedGameObject(GameObject*& selectedGameObject) { m_selectedGameObject = &selectedGameObject; }
    void DrawImGui() override;
};
