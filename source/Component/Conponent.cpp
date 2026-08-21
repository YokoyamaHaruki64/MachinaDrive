#include "pch.h"

#include "Component/ComponentInc.h"
#include "System/FieldRegistry.h"



Transform* Component::transform() const
{
    return m_owner ? m_owner->GetTransform() : nullptr;
}

void Component::DrawInspectorImGui()
{
    FieldRegistry fields;
    RegisterFields(fields);
    fields.DrawImGui();
}
