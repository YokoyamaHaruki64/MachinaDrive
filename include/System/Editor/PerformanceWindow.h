#pragma once

#include "System/Editor/EditorWindow.h"

// フレーム時間やオブジェクト数を表示するEditorウィンドウ。
class PerformanceWindow final : public EditorWindow
{
public:
    void DrawImGui() override;
};
