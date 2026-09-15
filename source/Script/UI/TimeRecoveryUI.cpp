#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/UI/TimeRecoveryUI.h"

#include "Script/UI/UIFade.h"
#include "Script/UI/UIOffset.h"

FIELDS_BEGIN(TimeRecoveryUI)
    GM_FIELD(m_recoveryText);
    GM_FIELD(m_duration);
FIELDS_END()

void TimeRecoveryUI::OnStart()
{
    m_duration = Math::Max(0.0f, m_duration);
    ResolveTargets();
    ApplyContent();
    ApplyProgress();
    ApplyVisibility();
}

void TimeRecoveryUI::Update()
{
    if (m_progress >= 1.0f)
    {
        return;
    }

    m_progress = m_duration > 0.0f
        ? Math::Min(1.0f, m_progress + Time::GetDeltaTime() / m_duration)
        : 1.0f;
    ApplyProgress();
    ApplyVisibility();
}

void TimeRecoveryUI::SetVisible(bool visible)
{
    m_visible = visible;
    ApplyVisibility();
}

void TimeRecoveryUI::ShowRecovery(float amount)
{
    if (!std::isfinite(amount) || amount <= 0.0f || m_duration <= 0.0f)
    {
        return;
    }

    m_roundedAmount = static_cast<int>(std::lround(amount));
    if (m_roundedAmount <= 0)
    {
        return;
    }

    m_progress = 0.0f;
    ResolveTargets();
    ApplyContent();
    ApplyProgress();
    ApplyVisibility();
}

void TimeRecoveryUI::ResolveTargets()
{
    if (!m_recoveryText)
    {
        return;
    }

    if (!m_text)
    {
        m_text = m_recoveryText->GetComponent<UIText>();
    }
    if (!m_fade)
    {
        m_fade = m_recoveryText->GetComponent<UIFade>();
    }
    if (!m_offset)
    {
        m_offset = m_recoveryText->GetComponent<UIOffset>();
    }
}

void TimeRecoveryUI::ApplyContent()
{
    if (m_text && m_roundedAmount > 0)
    {
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "+ %d Sec", m_roundedAmount);
        m_text->SetText(buffer);
    }
}

void TimeRecoveryUI::ApplyProgress()
{
    if (m_fade)
    {
        m_fade->SetProgress(m_progress);
    }
    if (m_offset)
    {
        m_offset->SetProgress(m_progress);
    }
}

void TimeRecoveryUI::ApplyVisibility()
{
    if (m_recoveryText)
    {
        m_recoveryText->SetActive(m_visible && m_progress < 1.0f);
    }
}
