#include "pch.h"

#include "imgui.h"

#include "Component/Transform.h"
#include "System/HierarchyWindow.h"
#include "System/Manager.h"

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

void HierarchyWindow::DrawImGui(GameObject*& selectedGameObject)
{
    ImGui::SetNextWindowSize(ImVec2(260.0f, 400.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Hierarchy");

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
        ImGui::EndPopup();
    }

    ImGui::End();
}
