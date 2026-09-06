#include "pch.h"

#include "Component/ComponentInc.h"
#include "System/Editor/FieldRegistry.h"



Transform* Component::transform() const
{
    return m_owner ? m_owner->GetTransform() : nullptr;
}

void Component::SetEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
    {
        return;
    }

    const bool wasEffectivelyEnabled = m_isEnabled
        && m_owner && m_owner->IsStarted() && m_owner->IsActive() && !m_owner->IsDestroyed();
    m_isEnabled = enabled;
    const bool isEffectivelyEnabled = m_isEnabled
        && m_owner && m_owner->IsStarted() && m_owner->IsActive() && !m_owner->IsDestroyed();

    if (wasEffectivelyEnabled && !isEffectivelyEnabled)
    {
        OnDisable();
    }
    else if (!wasEffectivelyEnabled && isEffectivelyEnabled)
    {
        OnEnable();
    }
}

bool Component::DrawInspectorImGui(
    InspectorFieldSpace inspectorSpace,
    bool* editStarted,
    bool* editFinished)
{
    FieldRegistry fields;
    // InspectorではComponentヘッダーに表示するため、ここでは保存だけ行う。
    fields.Add("enabled", m_isEnabled, InspectorFieldType::Hidden);
    RegisterFields(fields);
    return fields.DrawImGui(inspectorSpace, editStarted, editFinished);
}

bool Component::ToYAMLValue(YAMLValue& output, const YAMLValueConversionContext& context)
{
    FieldRegistry fields;
    fields.Add("enabled", m_isEnabled, InspectorFieldType::Hidden);
    RegisterFields(fields);
    return fields.ToYAMLValue(output, context);
}

bool Component::FromYAMLValue(const YAMLValue& input, const YAMLValueConversionContext& context)
{
    FieldRegistry fields;
    fields.Add("enabled", m_isEnabled, InspectorFieldType::Hidden);
    RegisterFields(fields);
    return fields.FromYAMLValue(input, context);
}
