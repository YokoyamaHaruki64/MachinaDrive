#pragma once

#include <windows.h>

#include "math/Vector2.h"

class Input;

// Inputだけが所有・利用するマウス入力の内部実装。
class Mouse final
{
    friend class Input;

    Mouse() = default;

    void Initialize(HWND window);
    void Shutdown();
    void Update(const BYTE* keyState);

    Vector2 GetPosition() const;
    // RenderSizeを基準に正規化し、各軸を0.0～1.0へクランプする。
    Vector2 GetNormalizedPosition() const;
    Vector2 GetPreviousPosition() const;
    Vector2 GetDelta() const;
    Vector2 GetNormalizedDelta() const;

    bool GetButtonPress(BYTE button) const;
    bool GetButtonTrigger(BYTE button) const;
    bool GetButtonRelease(BYTE button) const;

    bool SetPositionNormalized(const Vector2& normalizedPosition);
    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const;
    void SetCursorVisible(bool visible);
    bool IsCursorVisible() const;

    bool QueryMousePosition(Vector2& position) const;
    bool ApplyCursorClip() const;
    bool QueryCursorClipRect(RECT& rect) const;
    bool QueryCursorVisible() const;

    HWND m_window = nullptr;
    Vector2 m_position = Vector2::Zero();
    Vector2 m_previousPosition = Vector2::Zero();
    Vector2 m_delta = Vector2::Zero();
    bool m_hasPosition = false;

    static constexpr BYTE ButtonCount = 5;
    BYTE m_oldButtonState[ButtonCount]{};
    BYTE m_buttonState[ButtonCount]{};
    bool m_cursorLocked = false;
};
