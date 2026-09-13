#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/UI/WaveStartUI.h"

#include "Script/FillBar.h"
#include "Script/UI/UIFade.h"
#include "Script/WaveDirector.h"

FIELDS_BEGIN(WaveStartUI)
    GM_FIELD(m_frame);
    GM_FIELD(m_beginText);
    GM_FIELD(m_onDuration);
    GM_FIELD(m_offDuration);
FIELDS_END()

void WaveStartUI::OnStart()
{
    ResolveTargets();
    m_onDuration = std::isfinite(m_onDuration) ? Math::Max(m_onDuration, 0.0f) : 0.0f;
    m_offDuration = std::isfinite(m_offDuration) ? Math::Max(m_offDuration, 0.0f) : 0.0f;
    ApplyProgress();
    ApplyVisibility();
}

void WaveStartUI::SetVisible(bool visible)
{
    m_visible = visible;
    ApplyVisibility();
}

void WaveStartUI::SetProgress(float progress)
{
    m_progress = Math::Clamp(progress, 0.0f, 1.0f);
    ResolveTargets();

    if (m_state == State::Begin)
    {
        if (m_progress >= 1.0f)
        {
            m_state = State::Hidden;
        }
    }
    else
    {
        m_state = m_progress > 0.0f ? State::Countdown : State::Hidden;
    }

    ApplyProgress();
    ApplyVisibility();
}

void WaveStartUI::ShowBegin()
{
    m_progress = 0.0f;
    m_state = State::Begin;
    ResolveTargets();
    ApplyProgress();
    ApplyVisibility();
}

void WaveStartUI::SetBeginProgress(float progress)
{
    if (!std::isfinite(progress))
    {
        return;
    }

    m_progress = Math::Clamp(progress, 0.0f, 1.0f);
    m_state = m_progress >= 1.0f ? State::Hidden : State::Begin;
    ResolveTargets();

    if (!m_waveDirector)
    {
        if (GameObject* directorObject = Manager::GetFirstObjectByComponent<WaveDirector>())
        {
            m_waveDirector = directorObject->GetComponent<WaveDirector>();
        }
    }

    ApplyProgress();
    ApplyVisibility();
}

void WaveStartUI::ResolveTargets()
{
    if (m_frame && !m_progressBar)
    {
        for (auto* child : m_frame->GetTransformBase()->GetChildren())
        {
            if (child)
            {
                if (child->GetComponent<UIText>())
                {
                    m_progressTextObject = child->GetOwner();
                }
                else if (child->GetComponent<FillBar>())
                {
                    m_progressBar = child->GetComponent<FillBar>();
                }
            }
        }
    }
    if (m_beginText && !m_beginFade)
    {
        m_beginFade = m_beginText->GetComponent<UIFade>();
    }

}

void WaveStartUI::ApplyProgress()
{
    if (m_progressBar && m_state == State::Countdown)
    {
        m_progressBar->SetProgress(m_progress);
    }
    if (m_beginFade)
    {
        bool reverse = false;
        float beginProgress = 1.0f;
        if (m_state == State::Begin)
        {
            const float beginDuration = m_waveDirector ? m_waveDirector->GetWaveBeginDuration() : 0.0f;
            const float onDuration = Math::Min(m_onDuration, beginDuration);
            const float offDuration = Math::Min(m_offDuration, beginDuration - onDuration);
            const float elapsed = beginDuration * m_progress;

            if (onDuration > 0.0f && elapsed < onDuration)
            {
                reverse = true;
                beginProgress = elapsed / onDuration;
            }
            else if (offDuration > 0.0f && elapsed >= beginDuration - offDuration)
            {
                beginProgress = (elapsed - (beginDuration - offDuration)) / offDuration;
            }
            else
            {
                beginProgress = 0.0f;
            }
        }

        m_beginFade->SetReverse(reverse);
        m_beginFade->SetProgress(beginProgress);
    }
}

void WaveStartUI::ApplyVisibility()
{
    if (m_frame && m_progressBar && m_progressTextObject)
    {
        const bool progressVisible = m_visible && (m_state == State::Countdown);
        m_frame->SetActive(progressVisible);
        m_progressBar->GetOwner()->SetActive(progressVisible);
        m_progressTextObject->SetActive(progressVisible);
        
    }
    if (m_beginText)
    {
        m_beginText->SetActive(m_visible && m_state == State::Begin);
    }
}
