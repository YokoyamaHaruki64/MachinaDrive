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

    m_isEnabled = enabled;
    if (m_isEnabled && m_owner && m_owner->IsStarted() && m_owner->IsActive() && !m_owner->IsDestroyed())
    {
        OnEnable();
    }
}

void Component::DrawInspectorImGui(InspectorFieldSpace inspectorSpace)
{
    FieldRegistry fields;
    // InspectorではComponentヘッダーに表示するため、ここでは保存だけ行う。
    fields.Add("enabled", m_isEnabled, InspectorFieldType::Hidden);
    RegisterFields(fields);
    fields.DrawImGui(inspectorSpace);
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
