#pragma once

#include <vector>

#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"
#include "System/UI/UIRect.h"

class UITransform;

// Screen Space Overlay用の16:9 Canvas。UITransformの導出と入力問い合わせを一元化する。
class UICanvas final : public Component
{
    REGISTER_COMPONENT_OPTIONS(UICanvas, "UI Canvas", false, false);

    static constexpr float ReferenceWidth = 1920.0f;
    static constexpr float ReferenceHeight = 1080.0f;

    UIRect m_pixelRect{};
    float m_scale = 1.0f;

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    UICanvas() = delete;
    explicit UICanvas(GameObject* owner);

    void LateUpdate() override;

    float GetScale() const { return m_scale; }
    const UIRect& GetPixelRect() const { return m_pixelRect; }
    Vector2 GetPointerPosition() const;
    bool IsPointerTarget(const UITransform& target) const;
    bool GetOverlappingGameObjects(const UITransform& source, std::vector<GameObject*>& output) const;
};
