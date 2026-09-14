#pragma once

#include "Common/Color.h"
#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"

class UIImage;

enum class UIButtonState
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
};

// UIImageのPointer状態を利用し、状態別の色を反映する追加Component。
class UIButton final : public Component
{
    REGISTER_COMPONENT(UIButton, "UI Button");

    Color m_normalColor{ Color::White() };
    Color m_hoveredColor{ 0.9f, 0.9f, 0.9f, 1.0f };
    Color m_pressedColor{ 0.75f, 0.75f, 0.75f, 1.0f };
    Color m_disabledColor{ 0.5f, 0.5f, 0.5f, 1.0f };
    UIButtonState m_state{ UIButtonState::Normal };
    bool m_pressed{};
    bool m_released{};
    bool m_clicked{};

    void UpdateVisualState();
    const Color& GetStateColor() const;

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    UIButton() = delete;
    explicit UIButton(GameObject* owner);

    void OnLoaded() override;
    void OnInactive() override;
    void OnEditorUpdate() override;
    void Update() override;

    UIButtonState GetState() const { return m_state; }
    const Color& GetNormalColor() const { return m_normalColor; }
    const Color& GetHoveredColor() const { return m_hoveredColor; }
    const Color& GetPressedColor() const { return m_pressedColor; }
    const Color& GetDisabledColor() const { return m_disabledColor; }

    bool WasPressed() const { return m_pressed; }
    bool WasReleased() const { return m_released; }
    bool WasClicked() const { return m_clicked; }
};
