#include "pch.h"

#include "imgui.h"

#include "System/InspectorWindow.h"
#include "Component/Transform.h"
#include "Graphics/Material.h"
#include "System/RendererBase.h"
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
            InspectorFieldSpace inspectorSpace = InspectorFieldSpace::All;
            if (dynamic_cast<Transform*>(component))
            {
                ImGui::Checkbox("Local", &m_showLocalTransform);
                inspectorSpace = m_showLocalTransform ? InspectorFieldSpace::Local : InspectorFieldSpace::World;
            }

            component->DrawInspectorImGui(inspectorSpace);

            if (RendererBase* renderer = dynamic_cast<RendererBase*>(component))
            {
                for (uint32_t slot = 0; slot < renderer->GetMaterialSlotCount(); ++slot)
                {
                    if (Material* material = renderer->GetMaterial(slot))
                    {
                        ImGui::PushID(static_cast<int>(slot));
                        if (ImGui::TreeNode("Material"))
                        {
                            material->DrawInspectorImGui();
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                }
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }

    ImGui::End();
}

