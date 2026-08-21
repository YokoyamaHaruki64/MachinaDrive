#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "Graphics/GraphicInc.h"

enum class InspectorFieldType
{
    Auto,
    Slider,
    List,
    Curve,
};

enum class FieldValueType
{
    Bool,
    Int,
    Float,
    String,
    Vector2,
    Vector3,
    Vector4,
    Color,
    Matrix3x3,
    Matrix4x4,
};

struct FieldDescriptor
{
    std::string key;
    std::string displayName;
    void* value{};
    FieldValueType valueType{};
    InspectorFieldType inspectorType{ InspectorFieldType::Auto };
};

class FieldRegistry
{
    std::vector<FieldDescriptor> m_fields;

    static std::string CreateDisplayName(std::string_view key);

    template <typename T>
    static constexpr FieldValueType GetValueType()
    {
        if constexpr (std::is_same_v<T, bool>) return FieldValueType::Bool;
        else if constexpr (std::is_same_v<T, int>) return FieldValueType::Int;
        else if constexpr (std::is_same_v<T, float>) return FieldValueType::Float;
        else if constexpr (std::is_same_v<T, std::string>) return FieldValueType::String;
        else if constexpr (std::is_same_v<T, Vector2>) return FieldValueType::Vector2;
        else if constexpr (std::is_same_v<T, Vector3>) return FieldValueType::Vector3;
        else if constexpr (std::is_same_v<T, Vector4>) return FieldValueType::Vector4;
        else if constexpr (std::is_same_v<T, Color>) return FieldValueType::Color;
        else if constexpr (std::is_same_v<T, Matrix3x3>) return FieldValueType::Matrix3x3;
        else if constexpr (std::is_same_v<T, Matrix4x4>) return FieldValueType::Matrix4x4;
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
