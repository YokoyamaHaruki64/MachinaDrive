#pragma once

class GameObject;

class HierarchyWindow final
{
    void DrawGameObjectNode(GameObject& gameObject, GameObject*& selectedGameObject);
    void DrawRootDropTarget(GameObject*& selectedGameObject);

public:
    void DrawImGui(GameObject*& selectedGameObject);
};
