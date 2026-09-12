#include "ComboUI.h"

#include "Component/ComponentInc.h"

#include "Component/UIText.h"

#include <cmath>
#include <cstdio>
#include <numbers>

FIELDS_BEGIN(ComboUI)
    GM_FIELD(m_comboValue);
    GM_FIELD(m_boundTime);
    GM_FIELD(m_boundScale);
    GM_FIELD(m_boundCurve);
FIELDS_END()

void ComboUI::OnStart()
{
    m_comboTransform = GetComponent<UITransform>();
    m_comboText = m_comboValue.Get() ? m_comboValue->GetComponent<UIText>() : nullptr;
    if (!m_comboTransform || !m_comboText)
    {
        Logger::Error("ComboUI: HUD_ComboとCombo_Valueの参照を解決できません。");
        return;
    }

    m_boundTime = Math::Max(0.0f, m_boundTime);
    m_boundScale = Math::Max(0.0f, m_boundScale);
    m_baseScale = m_comboTransform->GetUIScale();
    ResetCombo();
}

void ComboUI::Update()
{

    if (!m_comboTransform || m_timer <= 0.0f)
    {
        return;
    }

    m_timer = Math::Max(0.0f, m_timer - Time::GetDeltaTime());
    const float normalizedTime = 1.0f - m_timer / m_boundTime;
    const float curveValue = m_boundCurve.Evaluate(normalizedTime);
    m_comboTransform->SetUIScale(m_baseScale * (1.0f + m_boundScale * curveValue));

    if (m_timer <= 0.0f)
    {
        m_comboTransform->SetUIScale(m_baseScale);
    }
}

void ComboUI::AddCombo()
{
    SetCount(m_comboCount + 1);
}

void ComboUI::SetCount(int count)
{
    m_comboCount = Math::Max(0, count);
    if (!m_comboText || !m_comboTransform)
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
        m_comboTransform->SetUIScale(m_baseScale);
    }
}

void ComboUI::ResetCombo()
{
    SetCount(0);
}
