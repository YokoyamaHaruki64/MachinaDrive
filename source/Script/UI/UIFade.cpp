#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/UI/UIFade.h"

FIELDS_BEGIN(UIFade)
    GM_FIELD(m_easeType);
    GM_FIELD(m_startAlpha);
    GM_FIELD(m_endAlpha);
    GM_FIELD(m_reverse);
    GM_FIELD(m_reverseEaseType);
FIELDS_END()

void UIFade::OnStart()
{
    m_startAlpha = std::isfinite(m_startAlpha) ? Math::Clamp(m_startAlpha, 0.0f, 1.0f) : 1.0f;
    m_endAlpha = std::isfinite(m_endAlpha) ? Math::Clamp(m_endAlpha, 0.0f, 1.0f) : 0.0f;
    ResolveTargets();

    if (m_image)
    {
        m_imageColor = m_image->GetColor();
    }
    if (m_text)
    {
        m_textColor = m_text->GetColor();
    }

    Apply();
}

void UIFade::SetProgress(float progress)
{
    if (!std::isfinite(progress))
    {
        return;
    }

    m_progress = Math::Clamp(progress, 0.0f, 1.0f);
    Apply();
}

void UIFade::SetReverse(bool reverse)
{
    m_reverse = reverse;
    Apply();
}

void UIFade::SetReverseEaseType(bool enabled)
{
    m_reverseEaseType = enabled;
    Apply();
}

void UIFade::ResolveTargets()
{
    if (!m_owner)
    {
        return;
    }

    if (!m_image)
    {
        m_image = m_owner->GetComponent<UIImage>();
    }
    if (!m_text)
    {
        m_text = m_owner->GetComponent<UIText>();
    }
}

void UIFade::Apply()
{
    EaseType easeType = Easing::ToEaseType(m_easeType);
    if (m_reverse && m_reverseEaseType)
    {
        easeType = Easing::Reverse(easeType);
    }

    const float progress = m_reverse ? 1.0f - m_progress : m_progress;
    const float alpha = Math::Lerp(
        m_startAlpha,
        m_endAlpha,
        Easing::Evaluate(progress, easeType));

    if (m_image)
    {
        Color color = m_imageColor;
        color.a *= alpha;
        m_image->SetColor(color);
    }
    if (m_text)
    {
        Color color = m_textColor;
        color.a *= alpha;
        m_text->SetColor(color);
    }
}
