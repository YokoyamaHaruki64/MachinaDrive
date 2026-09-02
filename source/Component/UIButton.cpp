#include "pch.h"

#include "Component/UIButton.h"

#include "Component/UIImage.h"
#include "System/Core/GameObject.h"
#include "System/Editor/FieldRegistry.h"

UIButton::UIButton(GameObject* owner)
    : Component(owner)
{
}

void UIButton::OnLoaded()
{
    UpdateVisualState();
}

void UIButton::OnEditorUpdate()
{
    UpdateVisualState();
}

const Color& UIButton::GetStateColor() const
{
    switch (m_state)
    {
    case UIButtonState::Hovered:
        return m_hoveredColor;
    case UIButtonState::Pressed:
        return m_pressedColor;
    case UIButtonState::Disabled:
        return m_disabledColor;
    case UIButtonState::Normal:
    default:
        return m_normalColor;
    }
}

void UIButton::UpdateVisualState()
{
    UIImage* const image = m_owner->GetComponent<UIImage>();
    if (!image)
    {
        m_state = UIButtonState::Disabled;
        return;
    }

    if (!IsEnabled() || !image->IsEnabled() || !image->IsRaycastTarget())
    {
        m_state = UIButtonState::Disabled;
    }
    else if (image->IsPointerPressed())
    {
        m_state = UIButtonState::Pressed;
    }
    else if (image->IsPointerOver())
    {
        m_state = UIButtonState::Hovered;
    }
    else
    {
        m_state = UIButtonState::Normal;
    }

    image->SetColor(GetStateColor());
}

void UIButton::Update()
{
    m_pressed = false;
    m_released = false;
    m_clicked = false;

    UIImage* const image = m_owner->GetComponent<UIImage>();
    if (!image)
    {
        m_state = UIButtonState::Disabled;
        return;
    }

    UpdateVisualState();
    m_pressed = image->WasPointerPressed();
    m_released = image->WasPointerReleased();
    m_clicked = m_released && image->IsPointerOver() && !image->WasDragEnded();
}

void UIButton::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_normalColor);
    GM_FIELD(m_hoveredColor);
    GM_FIELD(m_pressedColor);
    GM_FIELD(m_disabledColor);
}
