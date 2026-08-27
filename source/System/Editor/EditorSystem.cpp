#include "pch.h"

#include "System/Editor/EditorSystem.h"
#include "System/Core/manager.h"
#include "System/Editor/FileSystem.h"
#include "System/Editor/ImGuiStyleConfig.h"
#include "System/Diagnostics/Logger.h"

#include "imgui.h"

#include <array>

GameObject* EditorSystem::m_selectedGameObject{};
GameWindow EditorSystem::m_gameWindow{};
HierarchyWindow EditorSystem::m_hierarchyWindow{};
InspectorWindow EditorSystem::m_inspectorWindow{};
PerformanceWindow EditorSystem::m_performanceWindow{};
LogWindow EditorSystem::m_logWindow{};

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

void EditorSystem::ReloadStyle()
{
    if (!ImGuiStyleConfig::Reload())
    {
        return;
    }

    ImGuiStyleConfig::Apply();
    ImGui::GetIO().IniFilename = ImGuiStyleConfig::GetIniPathString().c_str();
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

    if (ImGui::BeginMenu("Settings"))
    {
        if (ImGui::MenuItem("Style Reload"))
        {
            ReloadStyle();
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void EditorSystem::DrawImGui()
{
    DrawToolbar();

    if (m_selectedGameObject && !IsActiveGameObject(m_selectedGameObject))
        m_selectedGameObject = nullptr;

    m_hierarchyWindow.SetSelectedGameObject(m_selectedGameObject);
    m_inspectorWindow.SetSelectedGameObject(m_selectedGameObject);

    const std::array<EditorWindow*, 5> windows = {
        &m_gameWindow,
        &m_hierarchyWindow,
        &m_inspectorWindow,
        &m_performanceWindow,
        &m_logWindow,
    };
    for (EditorWindow* window : windows)
    {
        window->DrawImGui();
    }
}
