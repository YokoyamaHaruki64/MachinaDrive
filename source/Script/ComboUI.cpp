#include "ComboUI.h"

#include "Component/ComponentInc.h"

#include "Component/UIText.h"


FIELDS_BEGIN(ComboUI)
    GM_FIELD(m_boundTime);
    GM_FIELD(m_boundScale);
    GM_FIELD(m_boundCurve);
FIELDS_END()

void ComboUI::OnStart()
{

    ResetCombo();
}

void ComboUI::Update()
{

    if( !m_comboText )
    {
        m_comboText = GetComponent<UIText>();
        if( !m_comboText )
        {
            Logger::Error("ComboUI: UIText component not found in the specified GameObject.");
            return;
        }
    }

    if( !m_comboTextTransform )
    {
        m_comboTextTransform = GetComponent<UITransform>();
        if( !m_comboTextTransform )
        {
            Logger::Error("ComboUI: UITransform component not found in the specified GameObject.");
            return;
        }
    }

    if(m_timer > 0.0f)
    {
        m_timer -= Time::GetDeltaTime();
        float t = 1.0f - (m_timer / m_boundTime);
        float curveValue = m_boundCurve.Evaluate(t);
        if(m_timer <= 0.0f)
        {
            m_timer = 0.0f;
            m_comboTextTransform->SetOffset(Vector2::Zero());
        }
        else
        {
            Vector2 offset = Vector2(0.0f, -m_boundScale * curveValue);
            m_comboTextTransform->SetOffset(offset);
        }
    }
}

void ComboUI::AddCombo()
{
    if(!m_comboText)
    {
        m_comboText = GetComponent<UIText>();
        if(!m_comboText)
        {
            Logger::Error("ComboUI: UIText component not found in the specified GameObject.");
            return;
        }
    }

    m_comboCount++;
    m_comboText->SetText(std::to_string(m_comboCount) + " Combo!");
    m_timer = m_boundTime;
}

void ComboUI::ResetCombo()
{
    m_comboCount = 0;
    if(m_comboText)
    {
        m_comboText->SetText("");
    }
}
