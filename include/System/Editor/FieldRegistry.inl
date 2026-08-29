#pragma once

#if GM_ENABLE_EDITOR
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#endif

#if GM_ENABLE_EDITOR
template <typename T>
bool FieldRegistry::DrawStructuredValue(std::string_view label, T& value)
{
    // DrawStructuredValueは、Reflect型、vector型、その他の値型を判定して描画する。
    if constexpr (requires { Reflect<T>::Members(); })
    {
        // 独自型はReflectのメンバをTreeNodeとして再帰表示する。
        const std::string displayName = CreateDisplayName(label);
        return DrawReflectValue(displayName, value);
    }
    else if constexpr (FieldRegistryVectorTraits<T>::IsVector)
    {
        // vectorはList UIを使い、要素がReflect型ならさらに再帰
        using ElementType = typename FieldRegistryVectorTraits<T>::ElementType;

        FieldDescriptor field{
            std::string(label),
            CreateDisplayName(label),
            &value,
            GetValueType<ElementType>(),
            InspectorFieldType::List,
            InspectorFieldSpace::All,
        };
        return DrawList(field, value);
    }
    else
    {
        // その他の値型は通常のDrawValueで描画する。
        FieldDescriptor field{
            std::string(label),
            CreateDisplayName(label),
            &value,
            GetValueType<T>(),
            InspectorFieldType::Auto,
            InspectorFieldSpace::All,
        };
        return DrawValue(field, &value);
    }
}

template <typename T>
bool FieldRegistry::DrawReflectValue(std::string_view label, T& value)
{
    const std::string displayName(label);
    if (!ImGui::TreeNode(displayName.c_str()))
    {
        return false;
    }

    bool changed = false;
    const auto members = Reflect<T>::Members();
    // applyでタプルの各メンバを展開し、DrawReflectMemberを呼び出す。
    std::apply(
        [&value, &changed](const auto&... member)
        {
            ((changed |= DrawReflectMember(value, member)), ...);
        },
        members);
    ImGui::TreePop();
    return changed;
}

template <typename Owner, typename Member>
bool FieldRegistry::DrawReflectMember(Owner& object, const Member& member)
{
    return DrawStructuredValue(member.first, object.*member.second);
}

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
            changed |= DrawStructuredValue(elementField.displayName, values[index]);

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
#else
template <typename T>
bool FieldRegistry::DrawStructuredValue(std::string_view, T&)
{
    return false;
}

template <typename T>
bool FieldRegistry::DrawReflectValue(std::string_view, T&)
{
    return false;
}

template <typename Owner, typename Member>
bool FieldRegistry::DrawReflectMember(Owner&, const Member&)
{
    return false;
}

template <typename T>
bool FieldRegistry::DrawList(const FieldDescriptor&, std::vector<T>&)
{
    return false;
}
#endif
