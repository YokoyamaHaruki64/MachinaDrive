#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Graphics/GraphicInc.h"
#include "Common/ValueType.h"

enum class InspectorFieldType
{
    Auto,
    Slider,
    List,
    Curve,
};

struct FieldDescriptor
{
    std::string key;
    std::string displayName;
    void* value{};
    ValueType valueType{};
    InspectorFieldType inspectorType{ InspectorFieldType::Auto };
};

class FieldRegistry
{
    std::vector<FieldDescriptor> m_fields;

    static std::string CreateDisplayName(std::string_view key);

    template <typename T>
    static constexpr ValueType GetValueType()
    {
        if constexpr (std::is_same_v<T, bool>) return ValueType::Bool;
        else if constexpr (std::is_same_v<T, int>) return ValueType::Int;
        else if constexpr (std::is_same_v<T, float>) return ValueType::Float;
        else if constexpr (std::is_same_v<T, std::string>) return ValueType::String;
        else if constexpr (std::is_same_v<T, Vector2>) return ValueType::Vector2;
        else if constexpr (std::is_same_v<T, Vector3>) return ValueType::Vector3;
        else if constexpr (std::is_same_v<T, Vector4>) return ValueType::Vector4;
        else if constexpr (std::is_same_v<T, Color>) return ValueType::Color;
        else if constexpr (std::is_same_v<T, Matrix3x3>) return ValueType::Matrix3x3;
        else if constexpr (std::is_same_v<T, Matrix4x4>) return ValueType::Matrix4x4;
        else static_assert(!sizeof(T), "未対応のField型です。");
    }

public:
    template <typename T>
    void Add(std::string_view key, T& value, InspectorFieldType inspectorType = InspectorFieldType::Auto)
    {
        m_fields.push_back({ std::string(key), CreateDisplayName(key), &value, GetValueType<T>(), inspectorType });
    }

    void DrawImGui();
};

#define GM_FIELD(member) fields.Add(#member, member)
#define GM_FIELD_KEY(key, member) fields.Add(key, member)
#define GM_FIELD_TYPE(member, type) fields.Add(#member, member, type)
