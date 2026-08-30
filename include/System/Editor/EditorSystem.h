#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "System/Editor/GameWindow.h"
#include "System/Editor/HierarchyWindow.h"
#include "System/Editor/InspectorWindow.h"
#include "System/Editor/LogWindow.h"
#include "System/Editor/PerformanceWindow.h"

class GameObject;
class Component;
struct YAMLValue;

class EditorCommand
{
public:
    virtual ~EditorCommand() = default;

    virtual void Execute() = 0;
    virtual void Undo() = 0;
};

class EditorSystem final
{
    static std::vector<GameObject*> m_selectedGameObjects;
    static GameObject* m_selectionAnchor;
    static GameWindow m_gameWindow;
    static HierarchyWindow m_hierarchyWindow;
    static InspectorWindow m_inspectorWindow;
    static PerformanceWindow m_performanceWindow;
    static LogWindow m_logWindow;
    static std::filesystem::path m_pendingScenePath;
    static bool m_openSceneConfirmationRequested;
    static bool m_applicationCloseConfirmationRequested;
    static std::vector<std::unique_ptr<EditorCommand>> m_undoCommands;
    static std::vector<std::unique_ptr<EditorCommand>> m_redoCommands;

    static bool IsActiveGameObject(GameObject* gameObject);
    static void ReloadStyle();
    static void OpenScene(const std::filesystem::path& path);
    static void DrawConfirmationDialogs();

public:
    static bool ApplyGameWindowRenderTargetSize();
    static bool IsGameWindowReady();
    static bool IsUIDisplayEnabled();
    static bool RequestApplicationClose();
    static bool CanUndo();
    static bool CanRedo();
    static bool Undo();
    static bool Redo();
    static void ClearCommandHistory();
    static void RecordExecutedCommand(std::unique_ptr<EditorCommand> command);
    static void RecordExecutedCommand(std::function<void()> execute, std::function<void()> undo);
    static bool CaptureComponentState(Component& component, YAMLValue& output);
    static void RecordComponentChange(Component& component, YAMLValue before, YAMLValue after);
    static void DrawImGui();

private:
    static void DrawToolbar();
};
