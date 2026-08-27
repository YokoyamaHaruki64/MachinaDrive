#pragma once

#include "System/Editor/GameWindow.h"
#include "System/Editor/HierarchyWindow.h"
#include "System/Editor/InspectorWindow.h"
#include "System/Editor/LogWindow.h"
#include "System/Editor/PerformanceWindow.h"

class GameObject;

class EditorSystem final
{
    static GameObject* m_selectedGameObject;
    static GameWindow m_gameWindow;
    static HierarchyWindow m_hierarchyWindow;
    static InspectorWindow m_inspectorWindow;
    static PerformanceWindow m_performanceWindow;
    static LogWindow m_logWindow;

    static bool IsActiveGameObject(GameObject* gameObject);

public:
    static bool ApplyGameWindowRenderTargetSize();
    static bool IsGameWindowReady();
    static void DrawImGui();

private:
    static void DrawToolbar();
};
