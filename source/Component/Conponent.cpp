#include "pch.h"

#include "Component/ComponentInc.h"
#include "System/FieldRegistry.h"



Transform* Component::transform() const
{
    return m_owner ? m_owner->GetTransform() : nullptr;
}

void Component::DrawInspectorImGui(InspectorFieldSpace inspectorSpace)
{
    FieldRegistry fields;
    RegisterFields(fields);
    fields.DrawImGui(inspectorSpace);
}

bool Component::ToYAMLValue(YAMLValue& output, const YAMLValueConversionContext& context)
{
    FieldRegistry fields;
    RegisterFields(fields);
    return fields.ToYAMLValue(output, context);
}

bool Component::FromYAMLValue(const YAMLValue& input, const YAMLValueConversionContext& context)
{
    FieldRegistry fields;
    RegisterFields(fields);
    return fields.FromYAMLValue(input, context);
}
