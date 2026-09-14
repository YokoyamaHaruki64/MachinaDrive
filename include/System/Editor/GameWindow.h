#pragma once

#include "System/Editor/EditorWindow.h"

// Game RenderTextureをEditor上に表示するウィンドウ。
class GameWindow final : public EditorWindow
{
public:
    bool ApplyRenderTargetSize();
    bool IsRenderTargetReady() const;
    bool IsGameWindowFocused() const { return m_gameFocused; }
    bool IsUIDisplayEnabled() const { return m_displayUI; }
    bool IsViewportHovered() const { return m_viewportHovered; }
    const Vector2& GetDisplayPosition() const { return m_displayPosition; }
    const Vector2& GetDisplaySize() const { return m_displaySize; }
    void DrawImGui() override;

private:
    enum class AspectMode
    {
        Free,
        Ratio16x9,
    };

    AspectMode m_aspectMode = AspectMode::Free;
    bool m_displayUI = false;
    bool m_showUIRects = false;
    bool m_viewportHovered = false;
    Vector2 m_displayPosition = Vector2::Zero();
    Vector2 m_displaySize = Vector2::Zero();
    bool m_gameFocused = true;

    void SetGameFocus(bool focused);
    void DrawUIRects(const Vector2& imagePosition, const Vector2& imageSize) const;
};
