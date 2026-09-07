#include "pch.h"

#include "System/Core/main.h"
#include "System/Core/Mouse.h"
#include "System/Core/input.h"


BYTE Input::m_OldKeyState[256];
BYTE Input::m_KeyState[256];
Mouse Input::m_mouse;


void Input::Init()
{

	memset( m_OldKeyState, 0, 256 );
	memset( m_KeyState, 0, 256 );

	m_mouse.Initialize(GetWindow());
}

void Input::Uninit()
{
	m_mouse.Shutdown();
}

void Input::Update()
{

	memcpy( m_OldKeyState, m_KeyState, 256 );

	GetKeyboardState( m_KeyState );

	m_mouse.Update(m_KeyState);
}

bool Input::GetKeyPress(BYTE KB)
{
	return (m_KeyState[KB] & 0x80);
}

bool Input::GetKeyRelease(BYTE KB)
{
	return (!(m_KeyState[KB] & 0x80) && (m_OldKeyState[KB] & 0x80));
}

bool Input::GetKeyTrigger(BYTE KB)
{
	return ((m_KeyState[KB] & 0x80) && !(m_OldKeyState[KB] & 0x80));
}

Vector2 Input::GetMousePosition()
{
	return m_mouse.GetPosition();
}

Vector2 Input::GetMousePositionNormalized()
{
	return m_mouse.GetNormalizedPosition();
}

Vector2 Input::GetMouseDelta()
{
	return m_mouse.GetDelta();
}

Vector2 Input::GetMouseDeltaNormalized()
{
	return m_mouse.GetNormalizedDelta();
}

Vector2 Input::GetMouseRawDelta()
{
	return m_mouse.GetRawDelta();
}

bool Input::SetMousePositionNormalized(const Vector2& normalizedPosition)
{
	return m_mouse.SetPositionNormalized(normalizedPosition);
}

bool Input::GetMousePress(MouseButton button)
{
	return m_mouse.GetButtonPress(static_cast<BYTE>(button));
}

bool Input::GetMouseTrigger(MouseButton button)
{
	return m_mouse.GetButtonTrigger(static_cast<BYTE>(button));
}

bool Input::GetMouseRelease(MouseButton button)
{
	return m_mouse.GetButtonRelease(static_cast<BYTE>(button));
}

void Input::ShowCursor(bool visible)
{
	m_mouse.SetCursorVisible(visible);
}

bool Input::IsCursorVisible()
{
	return m_mouse.IsCursorVisible();
}

namespace InputInternal
{
    void ProcessRawInput(HRAWINPUT rawInput)
    {
        Input::m_mouse.ProcessRawInput(rawInput);
    }

    void SetMouseWindowActive(bool active)
    {
        Input::m_mouse.SetWindowActive(active);
    }
}
