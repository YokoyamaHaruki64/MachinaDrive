#include "pch.h"

#include "System/Editor/EditorSystem.h"
#include "System/Core/manager.h"
#include "System/Editor/FileSystem.h"
#include "System/Diagnostics/Logger.h"

#include "imgui.h"

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

void EditorSystem::DrawToolbar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }

    const std::filesystem::path scenePath = Manager::GetCurrentSceneFilePath();
    const std::string sceneName = scenePath.empty() ? "(None)" : scenePath.filename().string();
    ImGui::Text("Scene: %s", sceneName.c_str());
    ImGui::Separator();

    if (!Manager::IsPlayMode())
    {
        if (ImGui::MenuItem("Open Scene"))
        {
            const std::optional<std::filesystem::path> selectedPath =
                FileSystem::OpenFileDialog("Assets/Scenes", ".scene");
            const std::optional<std::filesystem::path> projectPath = selectedPath
                ? FileSystem::ToProjectRelative(*selectedPath)
                : std::nullopt;
            if (projectPath && !Manager::LoadEditorScene(*projectPath))
            {
                Logger::Error("Sceneを読み込めません: " + projectPath->generic_string());
            }
        }
    }

    ImGui::EndMainMenuBar();
}

void EditorSystem::DrawImGui()
{
    DrawToolbar();

    m_gameWindow.DrawImGui();

    if (m_selectedGameObject && !IsActiveGameObject(m_selectedGameObject))
        m_selectedGameObject = nullptr;

    m_hierarchyWindow.DrawImGui(m_selectedGameObject);
    m_inspectorWindow.DrawImGui(m_selectedGameObject);
    m_debugWindow.DrawImGui();
}
