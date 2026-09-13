#pragma once

#include "Common/Color.h"
#include "System/Core/Script.h"

class UIImage;

// 現在値と被ダメージ量を、Bar色とChip色で表示するUIImage専用のゲージScript。
class FillBar final : public Script
{
    REGISTER_COMPONENT(FillBar, "Fill Bar");

    float m_max = 1.0f;
    float m_current = 1.0f;
    float m_chip = 0.0f;
    Color m_barColor{ Color::White() };
    Color m_chipColor{ 1.0f, 0.2f, 0.2f, 1.0f };
    float m_chipDecreaseSpeed = 1.0f;
    float m_chipDecreaseDelay = 0.5f;
    float m_chipDecreaseTimer = 0.0f;

    bool m_isDecreasing = false;

    bool m_verticalFill = false;
    bool m_horizontalFill = true;
    bool m_verticalFromTop = false;
    bool m_verticalFromCenter = false;
    bool m_verticalFromBottom = false;
    bool m_horizontalFromLeft = true;
    bool m_horizontalFromCenter = false;
    bool m_horizontalFromRight = false;

    UIImage* m_image{};

    void ClampValues();
    void ApplyMaterialProperties();

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    FillBar() = delete;
    explicit FillBar(GameObject* owner)
        : Script(owner)
    {
    }

    void OnStart() override;
    void Update() override;

    void SetMax(float value);
    void SetCurrent(float value);
    void AddCurrent(float value);
    void SetBarColor(const Color& color);
    void SetChipColor(const Color& color);
    void SetChipDecreaseSpeed(float value);
    void SetChipDecreaseDelay(float value);

    void SetProgress(float progress)
    {
        float clampedProgress = Math::Clamp(progress, 0.0f, 1.0f);
        SetCurrent(clampedProgress * m_max);
    }

    float GetMax() const { return m_max; }
    float GetCurrent() const { return m_current; }
    float GetProgress() const { return m_max > 0.0f ? m_current / m_max : 0.0f; }
    float GetChip() const { return m_chip; }
    const Color& GetBarColor() const { return m_barColor; }
    const Color& GetChipColor() const { return m_chipColor; }
    float GetChipDecreaseSpeed() const { return m_chipDecreaseSpeed; }
    float GetChipDecreaseDelay() const { return m_chipDecreaseDelay; }
};
