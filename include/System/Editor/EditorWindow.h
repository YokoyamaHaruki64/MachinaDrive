#pragma once

#include <cstdint>

#include "math/Vector2.h"

// ImGuiウィンドウ共通のサイズ取得とBegin/Endを扱う基底クラス。
class EditorWindow
{
public:
    virtual ~EditorWindow() = default;

    const Vector2& GetWindowSize() const { return m_windowSize; }
    const Vector2& GetContentSize() const { return m_contentSize; }
    bool IsLayoutReady() const { return m_layoutFrameCount >= 2; }

protected:
    bool BeginWindow(const char* title, const Vector2& initialSize);
    void UpdateContentSize();
    void EndWindow();

private:
    Vector2 m_windowSize = Vector2::Zero();
    Vector2 m_contentSize = Vector2::Zero();
    uint8_t m_layoutFrameCount = 0;
};
