#include "pch.h"

#include "imgui.h"

#include "System/InspectorWindow.h"
#include <typeinfo>

void InspectorWindow::DrawImGui(GameObject* selectedGameObject)
{
    ImGui::SetNextWindowSize(ImVec2(360.0f, 500.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    if (!selectedGameObject)
    {
        ImGui::TextUnformatted("GameObjectを選択してください。");
        ImGui::End();
        return;
    }

    ImGui::Text("Name: %s", selectedGameObject->GetName().c_str());
    ImGui::Text("Started: %s", selectedGameObject->IsStarted() ? "Yes" : "No");
    ImGui::Text("Destroy Requested: %s", selectedGameObject->IsDestroyed() ? "Yes" : "No");
    ImGui::Separator();

    for (Component* component : selectedGameObject->GetComponents())
    {
        ImGui::PushID(component);
        if (ImGui::TreeNode(typeid(*component).name()))
        {
            component->DrawInspectorImGui();
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::End();
}

