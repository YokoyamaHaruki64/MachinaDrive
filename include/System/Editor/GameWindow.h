#pragma once

#include "System/Editor/EditorWindow.h"

// Game RenderTextureをEditor上に表示するウィンドウ。
class GameWindow final : public EditorWindow
{
public:
    bool ApplyRenderTargetSize();
    bool IsRenderTargetReady() const;
    void DrawImGui();

private:
    enum class AspectMode
    {
        Free,
        Ratio16x9,
    };

    AspectMode m_aspectMode = AspectMode::Free;
    Vector2 m_displayPosition = Vector2::Zero();
    Vector2 m_displaySize = Vector2::Zero();
};
