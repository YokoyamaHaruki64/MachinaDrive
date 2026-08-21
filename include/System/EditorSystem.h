#pragma once

#include "System/HierarchyWindow.h"
#include "System/InspectorWindow.h"

class GameObject;

class EditorSystem final
{
    static GameObject* m_selectedGameObject;
    static HierarchyWindow m_hierarchyWindow;
    static InspectorWindow m_inspectorWindow;

    static bool IsActiveGameObject(GameObject* gameObject);

public:
    static void DrawImGui();
};
