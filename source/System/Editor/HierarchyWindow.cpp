#include "pch.h"

#include <optional>

#include "imgui.h"

#include "Component/Transform.h"
#include "System/Editor/FileSystem.h"
#include "System/Editor/HierarchyWindow.h"
#include "System/Diagnostics/Logger.h"
#include "System/Core/manager.h"

std::optional<std::filesystem::path> HierarchyWindow::ToAssetRelativePath(
    const std::filesystem::path& selectedPath)
{
    if (_wcsicmp(selectedPath.extension().c_str(), L".prefab") != 0)
    {
        Logger::Error("Prefabファイルの拡張子は.prefabにしてください。");
        return std::nullopt;
    }

    if (!FileSystem::IsUnderDirectory(selectedPath, "Assets"))
    {
        Logger::Error("PrefabはAssetsフォルダ配下に配置してください。");
        return std::nullopt;
    }

    return FileSystem::ToProjectRelative(selectedPath);
}

void HierarchyWindow::DrawGameObjectNode(GameObject& gameObject, GameObject*& selectedGameObject)
{
    Transform* transform = gameObject.GetTransform();
    const std::vector<Transform*>& children = transform->GetChildren();

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (children.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (selectedGameObject == &gameObject)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::PushID(&gameObject);
    const bool isOpen = ImGui::TreeNodeEx(gameObject.GetName().c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        selectedGameObject = &gameObject;
    }

    if (ImGui::BeginDragDropSource())
    {
        GameObject* draggedGameObject = &gameObject;
        ImGui::SetDragDropPayload("HierarchyGameObject", &draggedGameObject, sizeof(draggedGameObject));
        ImGui::TextUnformatted(gameObject.GetName().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HierarchyGameObject"))
        {
            GameObject* draggedGameObject = *static_cast<GameObject* const*>(payload->Data);
            if (draggedGameObject != &gameObject)
            {
                draggedGameObject->SetParent(&gameObject);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem("GameObjectContext"))
    {
        if (ImGui::MenuItem("Create Prefab"))
        {
            const std::optional<std::filesystem::path> selectedPath =
                FileSystem::SaveFileDialog("Assets", ".prefab", "New.prefab");
            if (selectedPath)
            {
                if (const std::optional<std::filesystem::path> assetPath = ToAssetRelativePath(*selectedPath))
                {
                    Manager::SavePrefab(gameObject, *assetPath);
                }
            }
        }
        if (ImGui::MenuItem("Delete"))
        {
            gameObject.Destroy();
            if (selectedGameObject == &gameObject)
            {
                selectedGameObject = nullptr;
            }
        }
        ImGui::EndPopup();
    }

    if (isOpen)
    {
        for (Transform* child : children)
        {
            DrawGameObjectNode(*child->GetOwner(), selectedGameObject);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void HierarchyWindow::DrawRootDropTarget(GameObject*& selectedGameObject)
{
    ImGui::Separator();
    ImGui::TextUnformatted("Root (drop here)");
    ImGui::InvisibleButton("RootDropTarget", ImVec2(-FLT_MIN, 28.0f));

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HierarchyGameObject"))
        {
            GameObject* draggedGameObject = *static_cast<GameObject* const*>(payload->Data);
            draggedGameObject->SetParent(nullptr);
            selectedGameObject = draggedGameObject;
        }
        ImGui::EndDragDropTarget();
    }
}

void HierarchyWindow::DrawImGui()
{
    if (!m_selectedGameObject)
    {
        return;
    }

    GameObject*& selectedGameObject = *m_selectedGameObject;
    BeginWindow("Hierarchy", Vector2(260.0f, 400.0f));

    if (ImGui::Button("Save Scene"))
    {
        Manager::SaveCurrentScene();
    }
    ImGui::SameLine();

    const std::filesystem::path scenePath = Manager::GetCurrentSceneFilePath();
    const std::string sceneName = scenePath.empty() ? "(None)" : scenePath.filename().string();
    ImGui::Text("Scene: %s", sceneName.c_str());
    ImGui::Separator();

    for (const std::unique_ptr<GameObject>& gameObject : Manager::GetActiveGameObjects())
    {
        if (!gameObject->GetTransform()->GetParent())
        {
            DrawGameObjectNode(*gameObject, selectedGameObject);
        }
    }

    DrawRootDropTarget(selectedGameObject);

    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty"))
        {
            selectedGameObject = Manager::CreateEmpty();
        }
        if (ImGui::MenuItem("Instantiate Prefab"))
        {
            const std::optional<std::filesystem::path> selectedPath =
                FileSystem::OpenFileDialog("Assets", ".prefab");
            if (selectedPath)
            {
                if (const std::optional<std::filesystem::path> assetPath = ToAssetRelativePath(*selectedPath))
                {
                    selectedGameObject = Manager::Instantiate(*assetPath);
                }
            }
        }
        ImGui::EndPopup();
    }

    EndWindow();
}
