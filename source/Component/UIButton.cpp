#include "pch.h"

#include "Component/UIButton.h"

#include "Component/UIImage.h"
#include "System/Core/GameObject.h"
#include "System/Editor/FieldRegistry.h"

UIButton::UIButton(GameObject* owner)
    : Component(owner)
{
}

void UIButton::Update()
{
    m_pressed = false;
    m_released = false;
    m_clicked = false;

    UIImage* const image = m_owner->GetComponent<UIImage>();
    if (!image)
    {
        return;
    }

    m_pressed = image->WasPointerPressed();
    m_released = image->WasPointerReleased();
    m_clicked = m_released && image->IsPointerOver() && !image->WasDragEnded();
}

void UIButton::RegisterFields(FieldRegistry& fields)
{
}
