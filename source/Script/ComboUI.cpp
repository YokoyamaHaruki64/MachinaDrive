#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/ComboUI.h"

FIELDS_BEGIN(ComboUI)
    GM_FIELD(m_comboValue);
    GM_FIELD(m_comboLabel);
    GM_FIELD(m_boundTime);
    GM_FIELD(m_boundScale);
    GM_FIELD(m_gap);
    GM_FIELD(m_boundCurve);
FIELDS_END()

void ComboUI::OnStart()
{
    m_comboText = m_comboValue.Get() ? m_comboValue->GetComponent<UIText>() : nullptr;
    m_comboLabelText = m_comboLabel.Get() ? m_comboLabel->GetComponent<UIText>() : nullptr;
    m_comboValueTransform = m_comboValue.Get() ? m_comboValue->GetComponent<UITransform>() : nullptr;
    m_comboLabelTransform = m_comboLabel.Get() ? m_comboLabel->GetComponent<UITransform>() : nullptr;
    if (!m_comboText || !m_comboLabelText || !m_comboValueTransform || !m_comboLabelTransform)
    {
        Logger::Error("ComboUI: Combo_ValueとCombo_Labelの参照を解決できません。");
        return;
    }

    m_boundTime = Math::Max(0.0f, m_boundTime);
    m_boundScale = Math::Max(0.0f, m_boundScale);
    m_gap = Math::Max(0.0f, m_gap);
    m_originalFontSize = m_comboText->GetFontSize();
    m_originalLabelFontSize = m_comboLabelText->GetFontSize();
    m_originalValueOffset = m_comboValueTransform->GetOffset();
    m_originalLabelOffset = m_comboLabelTransform->GetOffset();
    ResetCombo();
}

void ComboUI::ApplyScale(float scale)
{
    if (!m_comboText || !m_comboLabelText || !m_comboValueTransform || !m_comboLabelTransform)
    {
        return;
    }

    m_comboText->SetFontSize(m_originalFontSize * scale);
    m_comboLabelText->SetFontSize(m_originalLabelFontSize * scale);

    // 文字の向きに沿って、両方を同じ距離だけ外側へ開く。
    const float offsetAmount = m_gap * (scale - 1.0f);
    const float rotation = m_comboValueTransform->GetUIRotation();
    const Vector2 offset(std::cos(rotation) * offsetAmount, std::sin(rotation) * offsetAmount);
    m_comboValueTransform->SetOffset(m_originalValueOffset - offset);
    m_comboLabelTransform->SetOffset(m_originalLabelOffset + offset);
}

void ComboUI::Update()
{

    if (!m_comboText || m_timer <= 0.0f)
    {
        return;
    }

    m_timer = Math::Max(0.0f, m_timer - Time::GetDeltaTime());
    const float normalizedTime = 1.0f - m_timer / m_boundTime;
    const float curveValue = m_boundCurve.Evaluate(normalizedTime);
    ApplyScale(1.0f + m_boundScale * curveValue);

    if (m_timer <= 0.0f)
    {
        ApplyScale(1.0f);
    }
}

void ComboUI::AddCombo()
{
    SetCount(m_comboCount + 1);
}

void ComboUI::SetCount(int count)
{
    m_comboCount = Math::Max(0, count);
    if (!m_comboText)
    {
        return;
    }

    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%02d", m_comboCount);
    m_comboText->SetText(buffer);

    if (m_comboCount > 0 && m_boundTime > 0.0f)
    {
        m_timer = m_boundTime;
    }
    else
    {
        m_timer = 0.0f;
        ApplyScale(1.0f);
    }
}

void ComboUI::ResetCombo()
{
    SetCount(0);
}
