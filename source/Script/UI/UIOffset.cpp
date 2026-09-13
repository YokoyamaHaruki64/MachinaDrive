#include "pch.h"

#include "Script/UI/UIOffset.h"

#include "Common/Easing.h"
#include "Component/UITransform.h"
#include "System/Core/GameObject.h"
#include "System/Editor/FieldRegistry.h"

#include <cmath>

FIELDS_BEGIN(UIOffset)
    GM_FIELD(m_easeType);
    GM_FIELD(m_offset);
    GM_FIELD(m_reverse);
    GM_FIELD(m_reverseEaseType);
FIELDS_END()

void UIOffset::OnStart()
{
    ResolveTarget();
    if (m_transform)
    {
        m_baseOffset = m_transform->GetOffset();
    }

    Apply();
}

void UIOffset::SetProgress(float progress)
{
    if (!std::isfinite(progress))
    {
        return;
    }

    m_progress = Math::Clamp(progress, 0.0f, 1.0f);
    Apply();
}

void UIOffset::SetReverse(bool reverse)
{
    m_reverse = reverse;
    Apply();
}

void UIOffset::SetReverseEaseType(bool enabled)
{
    m_reverseEaseType = enabled;
    Apply();
}

void UIOffset::ResolveTarget()
{
    if (!m_transform && m_owner)
    {
        m_transform = m_owner->GetComponent<UITransform>();
    }
}

void UIOffset::Apply()
{
    if (m_transform)
    {
        EaseType easeType = Easing::ToEaseType(m_easeType);
        if (m_reverse && m_reverseEaseType)
        {
            easeType = Easing::Reverse(easeType);
        }

        const float rawProgress = m_reverse ? 1.0f - m_progress : m_progress;
        const float progress = Easing::Evaluate(rawProgress, easeType);
        m_transform->SetOffset(m_baseOffset + m_offset * progress);
    }
}
