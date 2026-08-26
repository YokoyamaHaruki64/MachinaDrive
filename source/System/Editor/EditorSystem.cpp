#include "pch.h"

#include "System/Editor/EditorSystem.h"
#include "System/Core/manager.h"

GameObject* EditorSystem::m_selectedGameObject{};
GameWindow EditorSystem::m_gameWindow{};
DebugWindow EditorSystem::m_debugWindow{};
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

bool EditorSystem::ApplyGameWindowRenderTargetSize()
{
    return m_gameWindow.ApplyRenderTargetSize();
}

bool EditorSystem::IsGameWindowReady()
{
    return m_gameWindow.IsRenderTargetReady();
}

void EditorSystem::DrawImGui()
{
    m_gameWindow.DrawImGui();

    if (m_selectedGameObject && !IsActiveGameObject(m_selectedGameObject))
        m_selectedGameObject = nullptr;

    m_hierarchyWindow.DrawImGui(m_selectedGameObject);
    m_inspectorWindow.DrawImGui(m_selectedGameObject);
    m_debugWindow.DrawImGui();
}
