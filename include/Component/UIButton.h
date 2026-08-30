#pragma once

#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"

class UIImage;

// UIImageのPointer状態を利用してクリック状態だけを提供する追加Component。
class UIButton final : public Component
{
    REGISTER_COMPONENT(UIButton, "UI Button");

    bool m_pressed{};
    bool m_released{};
    bool m_clicked{};

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    UIButton() = delete;
    explicit UIButton(GameObject* owner);

    void Update() override;

    bool WasPressed() const { return m_pressed; }
    bool WasReleased() const { return m_released; }
    bool WasClicked() const { return m_clicked; }
};
