#include "pch.h"

#include "imgui.h"

#include "System/HierarchyWindow.h"
#include "System/Manager.h"

void HierarchyWindow::DrawImGui(GameObject*& selectedGameObject)
{
    ImGui::SetNextWindowSize(ImVec2(260.0f, 400.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Hierarchy");

    for (const std::unique_ptr<GameObject>& gameObject : Manager::GetActiveGameObjects())
    {
        ImGui::PushID(gameObject.get());
        if (ImGui::Selectable(gameObject->GetName().c_str(), selectedGameObject == gameObject.get()))
            selectedGameObject = gameObject.get();
        ImGui::PopID();
    }

    ImGui::End();
}
