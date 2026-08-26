#pragma once

#include "System/Editor/DebugWindow.h"
#include "System/Editor/GameWindow.h"
#include "System/Editor/HierarchyWindow.h"
#include "System/Editor/InspectorWindow.h"

class GameObject;

class EditorSystem final
{
    static GameObject* m_selectedGameObject;
    static GameWindow m_gameWindow;
    static DebugWindow m_debugWindow;
    static HierarchyWindow m_hierarchyWindow;
    static InspectorWindow m_inspectorWindow;

    static bool IsActiveGameObject(GameObject* gameObject);

public:
    static bool ApplyGameWindowRenderTargetSize();
    static bool IsGameWindowReady();
    static void DrawImGui();
};
