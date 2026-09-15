#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/UI/HudNumber.h"

FIELDS_BEGIN(HudNumber)
    GM_FIELD(m_value);
FIELDS_END()

void HudNumber::OnStart()
{
    SetValue(m_value);
}

void HudNumber::SetValue(int value)
{
    m_value = (std::max)(0, value);
    ApplyDigits();
}

void HudNumber::ApplyDigits()
{
    const auto& children = m_owner->GetTransformBase()->GetChildren();
    int remaining = m_value;

    // 5列2行のアトラス。色は加工せず、桁のUV位置だけを指定する。
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        UIImage* image = (*it)->GetOwner()->GetComponent<UIImage>();
        if (!image)
        {
            continue;
        }

        const int digit = remaining % 10;
        remaining /= 10;
        auto& properties = image->GetMaterialPropertyOverride();
        properties.SetVector2("_UVScale", Vector2(0.2f, 0.5f));
        properties.SetVector2("_UVOffset", Vector2((digit % 5) * 0.2f, (digit / 5) * 0.5f));
    }
}
