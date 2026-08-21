#pragma once

class GameObject;

// 選択中GameObjectの詳細とComponentごとの項目を表示する。
class InspectorWindow final
{
public:
    void DrawImGui(GameObject* selectedGameObject);
};
