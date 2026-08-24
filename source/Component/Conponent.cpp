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

bool Component::EncodeFields(YAMLValue& output, const YAMLArchiveContext& context)
{
    FieldRegistry fields;
    RegisterFields(fields);
    return YAMLArchive::EncodeFields(fields, output, context);
}

bool Component::DecodeFields(const YAMLValue& input, const YAMLArchiveContext& context)
{
    FieldRegistry fields;
    RegisterFields(fields);
    return YAMLArchive::DecodeFields(input, fields, context);
}
