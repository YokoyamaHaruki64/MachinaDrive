#pragma once

#include <windows.h>

#include "math/Vector2.h"

class Input;

namespace InputInternal
{
    void ProcessRawInput(HRAWINPUT rawInput);
    void SetMouseWindowActive(bool active);
}

// Inputだけが所有・利用するマウス入力の内部実装。
class Mouse final
{
    friend class Input;
    friend void InputInternal::ProcessRawInput(HRAWINPUT rawInput);
    friend void InputInternal::SetMouseWindowActive(bool active);

    Mouse() = default;

    void Initialize(HWND window);
    void Shutdown();
    void Update(const BYTE* keyState);
    void ProcessRawInput(HRAWINPUT rawInput);
    void SetWindowActive(bool active);

    Vector2 GetPosition() const;
    // RenderSizeを基準に正規化し、各軸を0.0～1.0へクランプする。
    Vector2 GetNormalizedPosition() const;
    Vector2 GetDelta() const;
    Vector2 GetNormalizedDelta() const;
    Vector2 GetRawDelta() const;

    bool GetButtonPress(BYTE button) const;
    bool GetButtonTrigger(BYTE button) const;
    bool GetButtonRelease(BYTE button) const;

    bool SetPositionNormalized(const Vector2& normalizedPosition);
    void SetCursorVisible(bool visible);
    bool IsCursorVisible() const;

    bool QueryMousePosition(Vector2& position) const;
    bool ApplyCursorClip() const;
    bool QueryCursorClipRect(RECT& rect) const;
    bool QueryCursorVisible() const;
    // 表示状態とアクティブ状態から、カーソルのクリップ状態を更新する。
    void UpdateCursorClip() const;
    bool RegisterRawInput() const;
    void UnregisterRawInput() const;

    HWND m_window = nullptr;
    Vector2 m_position = Vector2::Zero();
    Vector2 m_previousPosition = Vector2::Zero();
    Vector2 m_delta = Vector2::Zero();
    Vector2 m_rawDelta = Vector2::Zero();
    Vector2 m_rawDeltaAccumulated = Vector2::Zero();
    bool m_hasPosition = false;
    bool m_cursorVisible = true;
    bool m_windowActive = false;

    static constexpr BYTE ButtonCount = 5;
    BYTE m_oldButtonState[ButtonCount]{};
    BYTE m_buttonState[ButtonCount]{};
};
