#pragma once

#include "System/Editor/EditorWindow.h"

// 実行時ログを表示・絞り込みするEditorウィンドウ。
class LogWindow final : public EditorWindow
{
public:
    void DrawImGui() override;
};
