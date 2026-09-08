#include "pch.h"

#include "Component/UIImage.h"
#include "Script/FillBar.h"
#include "System/Core/GM_Time.h"
#include "System/Editor/FieldRegistry.h"

void FillBar::ClampValues()
{
    m_max = Math::Max(m_max, 0.0f);
    m_current = Math::Clamp(m_current, 0.0f, m_max);
    m_chip = Math::Clamp(m_chip, 0.0f, m_max - m_current);
    m_chipDecreaseSpeed = Math::Max(m_chipDecreaseSpeed, 0.0f);
    m_chipDecreaseDelay = Math::Max(m_chipDecreaseDelay, 0.0f);
}

void FillBar::ApplyMaterialProperties()
{
    if (!m_image)
    {
        return;
    }

    const float barFill = m_max > 0.0f ? m_current / m_max : 0.0f;
    const float chipFill = m_max > 0.0f ? m_chip / m_max : 0.0f;
    // 縦横を同時に指定した場合は縦方向を優先し、未選択の方向はShaderへ渡さない。
    const bool useVerticalFill = m_verticalFill;
    const bool useHorizontalFill = !useVerticalFill && m_horizontalFill;

    MaterialPropertyOverride& properties = m_image->GetMaterialPropertyOverride();
    properties.SetFloat("Fill", barFill);
    properties.SetFloat("Chip", chipFill);
    properties.SetColor("BarColor", m_barColor);
    properties.SetColor("ChipColor", m_chipColor);
    properties.SetBool("VerticalFromTop", useVerticalFill && m_verticalFromTop);
    properties.SetBool("VerticalFromCenter", useVerticalFill && m_verticalFromCenter);
    properties.SetBool("VerticalFromBottom", useVerticalFill && m_verticalFromBottom);
    properties.SetBool("HorizontalFromLeft", useHorizontalFill && m_horizontalFromLeft);
    properties.SetBool("HorizontalFromCenter", useHorizontalFill && m_horizontalFromCenter);
    properties.SetBool("HorizontalFromRight", useHorizontalFill && m_horizontalFromRight);
}

void FillBar::RegisterFields(FieldRegistry& fields)
{
    GM_FIELD(m_max);
    GM_FIELD(m_current);

    GM_FIELD(m_barColor);
    GM_FIELD(m_chipColor);

    GM_FIELD(m_chipDecreaseSpeed);
    GM_FIELD(m_chipDecreaseDelay);

    GM_FIELD(m_verticalFill);
    GM_FIELD(m_horizontalFill);
    GM_FIELD(m_verticalFromTop);
    GM_FIELD(m_verticalFromCenter);
    GM_FIELD(m_verticalFromBottom);
    GM_FIELD(m_horizontalFromLeft);
    GM_FIELD(m_horizontalFromCenter);
    GM_FIELD(m_horizontalFromRight);
}

void FillBar::OnStart()
{
    m_image = GetComponent<UIImage>();
    ClampValues();
    ApplyMaterialProperties();
}

void FillBar::Update()
{
    if (m_chip > 0.0f)
    {
        m_chipDecreaseTimer -= Time::GetDeltaTime();
        if (m_chipDecreaseTimer <= 0.0f)
        {
            m_chip = Math::Max(0.0f, m_chip - m_chipDecreaseSpeed * Time::GetDeltaTime());
            m_isDecreasing = true;
        }
    }
    else
    {
        m_isDecreasing = false;
    }

    ClampValues();
    ApplyMaterialProperties();
}

void FillBar::SetMax(float value)
{
    m_max = value;
    ClampValues();
    ApplyMaterialProperties();
}

void FillBar::SetCurrent(float value)
{
    m_current = value;
    ClampValues();
    ApplyMaterialProperties();
}

void FillBar::AddCurrent(float value)
{
    const float previousCurrent = m_current;
    m_current += value;
    ClampValues();

    if (m_current < previousCurrent)
    {
        // 実際に減った量だけをChipへ加算し、連続被ダメージを累積表示する。
        m_chip += previousCurrent - m_current;

        // 減少中はディレイを入れない
        if (!m_isDecreasing)
        {
            m_chipDecreaseTimer = m_chipDecreaseDelay; // 被ダメージ表示の遅延タイマーをリセット
        }
    }

    ClampValues();
    ApplyMaterialProperties();
}

void FillBar::SetBarColor(const Color& color)
{
    m_barColor = color;
    ApplyMaterialProperties();
}

void FillBar::SetChipColor(const Color& color)
{
    m_chipColor = color;
    ApplyMaterialProperties();
}

void FillBar::SetChipDecreaseSpeed(float value)
{
    m_chipDecreaseSpeed = value;
    ClampValues();
}

void FillBar::SetChipDecreaseDelay(float value)
{
    m_chipDecreaseDelay = value;
    ClampValues();
}
