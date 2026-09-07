#include "pch.h"

#include "System/Core/Mouse.h"
#include "System/Rendering/renderer.h"

#if GM_ENABLE_EDITOR
#include "System/Editor/EditorSystem.h"
#endif

namespace
{
    float ClampUnit(float value)
    {
        return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    }
}

void Mouse::Initialize(HWND window)
{
    m_window = window;
    m_position = Vector2::Zero();
    m_previousPosition = Vector2::Zero();
    m_delta = Vector2::Zero();
    m_hasPosition = false;
    memset(m_oldButtonState, 0, sizeof(m_oldButtonState));
    memset(m_buttonState, 0, sizeof(m_buttonState));
    m_cursorLocked = false;

    Vector2 initialPosition;
    if (QueryMousePosition(initialPosition))
    {
        m_position = initialPosition;
        m_previousPosition = initialPosition;
        m_hasPosition = true;
    }
}

void Mouse::Shutdown()
{
    m_window = nullptr;
    m_position = Vector2::Zero();
    m_previousPosition = Vector2::Zero();
    m_delta = Vector2::Zero();
    m_hasPosition = false;
    memset(m_oldButtonState, 0, sizeof(m_oldButtonState));
    memset(m_buttonState, 0, sizeof(m_buttonState));
    m_cursorLocked = false;
    ::ClipCursor(nullptr);
}

void Mouse::Update(const BYTE* keyState)
{
    if (m_cursorLocked)
    {
        ApplyCursorClip();
    }

    memcpy(m_oldButtonState, m_buttonState, sizeof(m_buttonState));

    constexpr BYTE virtualKeyCodes[ButtonCount] =
    {
        VK_LBUTTON,
        VK_RBUTTON,
        VK_MBUTTON,
        VK_XBUTTON1,
        VK_XBUTTON2,
    };
    if (keyState)
    {
        for (BYTE button = 0; button < ButtonCount; ++button)
        {
            m_buttonState[button] = keyState[virtualKeyCodes[button]];
        }
    }
    else
    {
        memset(m_buttonState, 0, sizeof(m_buttonState));
    }

    Vector2 currentPosition;
    if (!QueryMousePosition(currentPosition))
    {
        // ウィンドウが無効なフレームは、誤った大きな移動量を発生させない。
        m_delta = Vector2::Zero();
        return;
    }

    if (!m_hasPosition)
    {
        m_position = currentPosition;
        m_previousPosition = currentPosition;
        m_delta = Vector2::Zero();
        m_hasPosition = true;
        return;
    }

    m_previousPosition = m_position;
    m_position = currentPosition;
    m_delta = m_position - m_previousPosition;
}

Vector2 Mouse::GetPosition() const
{
    return m_position;
}

Vector2 Mouse::GetNormalizedPosition() const
{
    const float renderWidth = static_cast<float>(Renderer::GetGameWidth());
    const float renderHeight = static_cast<float>(Renderer::GetGameHeight());
    if (renderWidth <= 0.0f || renderHeight <= 0.0f)
    {
        return Vector2::Zero();
    }

    const float normalizedX = m_position.x / renderWidth;
    const float normalizedY = m_position.y / renderHeight;
    return Vector2(
        ClampUnit(normalizedX),
        ClampUnit(normalizedY));
}

Vector2 Mouse::GetPreviousPosition() const
{
    return m_previousPosition;
}

Vector2 Mouse::GetDelta() const
{
    return m_delta;
}

Vector2 Mouse::GetNormalizedDelta() const
{
    const float renderWidth = static_cast<float>(Renderer::GetGameWidth());
    const float renderHeight = static_cast<float>(Renderer::GetGameHeight());
    if (renderWidth <= 0.0f || renderHeight <= 0.0f)
    {
        return Vector2::Zero();
    }

    return Vector2(
        m_delta.x / renderWidth,
        m_delta.y / renderHeight);
}

bool Mouse::SetPositionNormalized(const Vector2& normalizedPosition)
{
    if (!m_window || !::IsWindow(m_window))
    {
        return false;
    }

    const float renderWidth = static_cast<float>(Renderer::GetGameWidth());
    const float renderHeight = static_cast<float>(Renderer::GetGameHeight());
    if (renderWidth <= 0.0f || renderHeight <= 0.0f)
    {
        return false;
    }

    const Vector2 clampedPosition(
        ClampUnit(normalizedPosition.x),
        ClampUnit(normalizedPosition.y));
    POINT screenPosition{};

#if GM_ENABLE_EDITOR
    Vector2 displayPosition;
    Vector2 displaySize;
    if (EditorSystem::GetGameWindowDisplayRect(displayPosition, displaySize)
        && displaySize.x > 0.0f && displaySize.y > 0.0f)
    {
        screenPosition.x = static_cast<LONG>(std::lround(
            displayPosition.x + displaySize.x * clampedPosition.x));
        screenPosition.y = static_cast<LONG>(std::lround(
            displayPosition.y + displaySize.y * clampedPosition.y));
    }
    else
#endif
    {
        screenPosition.x = static_cast<LONG>(std::lround(renderWidth * clampedPosition.x));
        screenPosition.y = static_cast<LONG>(std::lround(renderHeight * clampedPosition.y));
        if (!::ClientToScreen(m_window, &screenPosition))
        {
            return false;
        }
    }

    if (!::SetCursorPos(screenPosition.x, screenPosition.y))
    {
        return false;
    }

    const Vector2 position(
        renderWidth * clampedPosition.x,
        renderHeight * clampedPosition.y);
    m_position = position;
    m_previousPosition = position;
    m_delta = Vector2::Zero();
    m_hasPosition = true;
    return true;
}

bool Mouse::GetButtonPress(BYTE button) const
{
    return button < ButtonCount && (m_buttonState[button] & 0x80) != 0;
}

bool Mouse::GetButtonTrigger(BYTE button) const
{
    return button < ButtonCount
        && (m_buttonState[button] & 0x80) != 0
        && (m_oldButtonState[button] & 0x80) == 0;
}

bool Mouse::GetButtonRelease(BYTE button) const
{
    return button < ButtonCount
        && (m_buttonState[button] & 0x80) == 0
        && (m_oldButtonState[button] & 0x80) != 0;
}

void Mouse::SetCursorLocked(bool locked)
{
    m_cursorLocked = locked;
    if (!locked)
    {
        ::ClipCursor(nullptr);
        return;
    }

    ApplyCursorClip();
}

bool Mouse::IsCursorLocked() const
{
    return m_cursorLocked;
}

void Mouse::SetCursorVisible(bool visible)
{
    if (QueryCursorVisible() == visible)
    {
        return;
    }

    // ShowCursorは表示カウンター方式のため、指定状態になるまで調整する。
    int displayCount = 0;
    do
    {
        displayCount = ::ShowCursor(visible ? TRUE : FALSE);
    } while ((visible && displayCount < 0) || (!visible && displayCount >= 0));
}

bool Mouse::IsCursorVisible() const
{
    return QueryCursorVisible();
}

bool Mouse::ApplyCursorClip() const
{
    RECT clipRect{};
    if (!QueryCursorClipRect(clipRect))
    {
        ::ClipCursor(nullptr);
        return false;
    }

    return ::ClipCursor(&clipRect) != FALSE;
}

bool Mouse::QueryCursorClipRect(RECT& rect) const
{
    if (!m_window || !::IsWindow(m_window))
    {
        return false;
    }

#if GM_ENABLE_EDITOR
    Vector2 displayPosition;
    Vector2 displaySize;
    if (EditorSystem::GetGameWindowDisplayRect(displayPosition, displaySize)
        && displaySize.x > 0.0f && displaySize.y > 0.0f)
    {
        rect.left = static_cast<LONG>(std::lround(displayPosition.x));
        rect.top = static_cast<LONG>(std::lround(displayPosition.y));
        rect.right = static_cast<LONG>(std::lround(displayPosition.x + displaySize.x));
        rect.bottom = static_cast<LONG>(std::lround(displayPosition.y + displaySize.y));
        return rect.right > rect.left && rect.bottom > rect.top;
    }
#endif

    RECT clientRect{};
    if (!::GetClientRect(m_window, &clientRect))
    {
        return false;
    }

    POINT topLeft{ clientRect.left, clientRect.top };
    POINT bottomRight{ clientRect.right, clientRect.bottom };
    if (!::ClientToScreen(m_window, &topLeft)
        || !::ClientToScreen(m_window, &bottomRight))
    {
        return false;
    }

    rect.left = topLeft.x;
    rect.top = topLeft.y;
    rect.right = bottomRight.x;
    rect.bottom = bottomRight.y;
    return rect.right > rect.left && rect.bottom > rect.top;
}

bool Mouse::QueryMousePosition(Vector2& position) const
{
    if (!m_window || !::IsWindow(m_window))
    {
        return false;
    }

    POINT cursorPosition{};
    if (!::GetCursorPos(&cursorPosition))
    {
        return false;
    }

#if GM_ENABLE_EDITOR
    Vector2 displayPosition;
    Vector2 displaySize;
    if (EditorSystem::GetGameWindowDisplayRect(displayPosition, displaySize))
    {
        const float renderWidth = static_cast<float>(Renderer::GetGameWidth());
        const float renderHeight = static_cast<float>(Renderer::GetGameHeight());

        if (renderWidth > 0.0f && renderHeight > 0.0f
            && displaySize.x > 0.0f && displaySize.y > 0.0f)
        {
            const Vector2 screenPosition(
                static_cast<float>(cursorPosition.x),
                static_cast<float>(cursorPosition.y));
            const Vector2 displayLocalPosition = screenPosition - displayPosition;

            position = Vector2(
                displayLocalPosition.x * renderWidth / displaySize.x,
                displayLocalPosition.y * renderHeight / displaySize.y);
            return true;
        }
    }
#endif

    if (!::ScreenToClient(m_window, &cursorPosition))
    {
        return false;
    }

    position = Vector2(
        static_cast<float>(cursorPosition.x),
        static_cast<float>(cursorPosition.y));
    return true;
}

bool Mouse::QueryCursorVisible() const
{
    CURSORINFO cursorInfo{ sizeof(CURSORINFO) };
    if (!::GetCursorInfo(&cursorInfo))
    {
        return false;
    }

    return (cursorInfo.flags & CURSOR_SHOWING) != 0;
}
