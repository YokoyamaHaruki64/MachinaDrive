#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#include "System/Editor/EditorWindow.h"

class GameObject;

class HierarchyWindow final : public EditorWindow
{
    struct SelectionRequest
    {
        GameObject* gameObject = nullptr;
        bool toggle = false;
        bool range = false;
    };

    std::vector<GameObject*>* m_selectedGameObjects = nullptr;
    GameObject** m_selectionAnchor = nullptr;
    std::vector<GameObject*> m_visibleGameObjects;
    std::vector<GameObject*> m_draggedGameObjects;
    std::optional<SelectionRequest> m_selectionRequest;

    void DrawGameObjectNode(GameObject& gameObject);
    void DrawRootDropTarget();
    void ApplySelectionRequest();
    void DeleteSelectedGameObjects();
    void CreateUIObject(std::string_view name, bool addText, bool addImage, bool addButton);
    void ApplyParentToDraggedGameObjects(
        const std::vector<GameObject*>& draggedGameObjects,
        GameObject* newParent);
    static std::optional<std::filesystem::path> ToAssetRelativePath(const std::filesystem::path& selectedPath);

public:
    void SetSelectedGameObjects(std::vector<GameObject*>& selectedGameObjects, GameObject*& selectionAnchor)
    {
        m_selectedGameObjects = &selectedGameObjects;
        m_selectionAnchor = &selectionAnchor;
    }
    void DrawImGui() override;
};
