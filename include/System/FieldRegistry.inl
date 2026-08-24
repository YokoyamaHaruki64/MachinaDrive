#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"

template <typename T>
bool FieldRegistry::DrawList(const FieldDescriptor& field, std::vector<T>& values)
{
    bool changed = false;
    if (ImGui::TreeNode(field.displayName.c_str()))
    {
        for (size_t index = 0; index < values.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));

            FieldDescriptor elementField = field;
            elementField.displayName = "Value";
            elementField.inspectorType = InspectorFieldType::Auto;
            changed |= DrawValue(elementField, &values[index]);

            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
            {
                values.erase(values.begin() + index);
                changed = true;
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }

        if (ImGui::Button("Add"))
        {
            values.emplace_back();
            changed = true;
        }
        ImGui::TreePop();
    }
    return changed;
}

template <>
inline bool FieldRegistry::DrawList<bool>(const FieldDescriptor& field, std::vector<bool>& values)
{
    bool changed = false;
    if (ImGui::TreeNode(field.displayName.c_str()))
    {
        for (size_t index = 0; index < values.size(); ++index)
        {
            ImGui::PushID(static_cast<int>(index));

            bool elementValue = values[index];
            if (ImGui::Checkbox("Value", &elementValue))
            {
                values[index] = elementValue;
                changed = true;
            }

            ImGui::SameLine();
            if (ImGui::SmallButton("Remove"))
            {
                values.erase(values.begin() + index);
                changed = true;
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }

        if (ImGui::Button("Add"))
        {
            values.emplace_back(false);
            changed = true;
        }
        ImGui::TreePop();
    }
    return changed;
}
