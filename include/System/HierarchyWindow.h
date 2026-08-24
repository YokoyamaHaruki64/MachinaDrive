#pragma once

#include <filesystem>
#include <optional>

class GameObject;

class HierarchyWindow final
{
    void DrawGameObjectNode(GameObject& gameObject, GameObject*& selectedGameObject);
    void DrawRootDropTarget(GameObject*& selectedGameObject);
    static std::optional<std::filesystem::path> ToAssetRelativePath(const std::filesystem::path& selectedPath);

public:
    void DrawImGui(GameObject*& selectedGameObject);
};
