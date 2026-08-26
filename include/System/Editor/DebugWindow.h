#pragma once

#include "System/Editor/EditorWindow.h"

// デバッグ用ImGuiウィンドウと各タブの配置を管理する。
class DebugWindow final : public EditorWindow
{
public:
    void DrawImGui();
};
