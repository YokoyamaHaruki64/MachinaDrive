#include "pch.h"

#include "System/EditorSystem.h"
#include "System/Manager.h"

GameObject* EditorSystem::m_selectedGameObject{};
HierarchyWindow EditorSystem::m_hierarchyWindow{};
InspectorWindow EditorSystem::m_inspectorWindow{};

bool EditorSystem::IsActiveGameObject(GameObject* gameObject)
{
    for (const std::unique_ptr<GameObject>& candidate : Manager::GetActiveGameObjects())
    {
        if (candidate.get() == gameObject && !candidate->IsDestroyed())
            return true;
    }
    return false;
}

void EditorSystem::DrawImGui()
{
    if (m_selectedGameObject && !IsActiveGameObject(m_selectedGameObject))
        m_selectedGameObject = nullptr;

    m_hierarchyWindow.DrawImGui(m_selectedGameObject);
    m_inspectorWindow.DrawImGui(m_selectedGameObject);
}
