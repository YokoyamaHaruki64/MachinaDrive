#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "Graphics/GraphicInc.h"
#include "Common/ValueType.h"
#include "math/Quaternion.h"
#include "System/Core/GameObjectReference.h"
#include "System/Core/PrefabReference.h"
#include "System/Serialization/YAMLValueConverter.h"

struct ImGuiInputTextCallbackData;

enum class InspectorFieldType
{
    Auto,
    Slider,
    List,
    Curve,
    Hidden,
};

enum class InspectorFieldSpace
{
    All,
    World,
    Local,
};

struct FieldDescriptor
{
    std::string key;
    std::string displayName;
    void* value{};
    ValueType valueType{};
    InspectorFieldType inspectorType{ InspectorFieldType::Auto };
    InspectorFieldSpace inspectorSpace{ InspectorFieldSpace::All };
    std::function<void(const FieldDescriptor&)> drawAccessor;
    std::function<YAMLValueConversionResult(YAMLValue&, const YAMLValueConversionContext&)> toYAMLValue;
    std::function<YAMLValueConversionResult(
        const YAMLValue&,
        const YAMLValueConversionContext&,
        std::function<void()>&)> fromYAMLValue;
};

template <typename T>
struct FieldRegistryVectorTraits
{
    // vector以外は通常の値として描画する。
    static constexpr bool IsVector = false;
};

template <typename T>
struct FieldRegistryVectorTraits<std::vector<T>>
{
    // Reflect型のvectorも、要素側の描画処理へ再帰できるようにする。
    static constexpr bool IsVector = true;
    using ElementType = T;
};

class FieldRegistry
{
    std::vector<FieldDescriptor> m_fields;

    static std::string CreateDisplayName(std::string_view key);
    static bool DrawValue(const FieldDescriptor& field, void* value);
    static bool DrawQuaternion(const char* label, Quaternion& value);
    static bool DrawGameObjectReference(const char* label, GameObjectReference& value);
    static bool DrawPrefabReference(const char* label, PrefabReference& value);
    static void DrawMatrix3x3(const char* label, Matrix3x3& value);
    static void DrawMatrix4x4(const char* label, Matrix4x4& value);
    template <typename T>
    static bool DrawStructuredValue(std::string_view label, T& value);

    template <typename T>
    static bool DrawReflectValue(std::string_view label, T& value);

    template <typename Owner, typename Member>
    static bool DrawReflectMember(Owner& object, const Member& member);

    static bool InputText(const char* label, std::string& value);
    static int ResizeStringInputCallback(ImGuiInputTextCallbackData* data);
    static void ReportConversionMessage(const YAMLValueConversionResult& result, std::string_view fieldKey);

    template <typename T>
    static bool DrawList(const FieldDescriptor& field, std::vector<T>& values);

    template <typename T>
    static void ConfigureSerialization(FieldDescriptor& descriptor, T& value)
    {
        T* target = &value;
        descriptor.toYAMLValue = [target](YAMLValue& output, const YAMLValueConversionContext& context)
        {
            return YAMLValueConverter<T>::ToYAMLValue(*target, output, context);
        };
        descriptor.fromYAMLValue = [target](
            const YAMLValue& input,
            const YAMLValueConversionContext& context,
            std::function<void()>& assignment)
        {
            T value{};
            YAMLValueConversionResult result = YAMLValueConverter<T>::FromYAMLValue(input, value, context);
            if (result.success)
            {
                assignment = [target, value = std::move(value)]() mutable
                {
                    *target = std::move(value);
                };
            }
            return result;
        };
    }

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
        else if constexpr (std::is_same_v<T, Quaternion>) return ValueType::Quaternion;
        else if constexpr (std::is_same_v<T, Curve<float>>) return ValueType::CurveFloat;
        else if constexpr (std::is_same_v<T, CurveColor>) return ValueType::CurveColor;
        else if constexpr (std::is_same_v<T, GameObjectReference>) return ValueType::GameObjectReference;
        else if constexpr (std::is_same_v<T, PrefabReference>) return ValueType::PrefabReference;
        // Reflect型は専用の再帰描画を使うため、ValueTypeの列挙には追加しない。
        else if constexpr (requires { Reflect<T>::Members(); }) return ValueType::Unknown;
        else static_assert(!sizeof(T), "未対応のField型です。");
    }

public:
    template <typename T>
    void Add(std::string_view key, T& value, InspectorFieldType inspectorType = InspectorFieldType::Auto)
    {
        FieldDescriptor descriptor{
            std::string(key),
            CreateDisplayName(key),
            &value,
            GetValueType<T>(),
            inspectorType,
        };

        if constexpr (requires { Reflect<T>::Members(); })
        {
            // Fieldは1つのまま保持し、Inspectorだけをメンバ単位で展開する。
            // YAMLはConfigureSerializationがReflect用Converterへ委譲するため、Map構造を維持できる。
            T* target = &value;
            descriptor.drawAccessor = [target](const FieldDescriptor& field)
            {
                DrawReflectValue(field.displayName, *target);
            };
        }

        ConfigureSerialization(descriptor, value);
        m_fields.push_back(std::move(descriptor));
    }

    template <typename T>
    void Add(std::string_view key, std::vector<T>& values, InspectorFieldType inspectorType = InspectorFieldType::List)
    {
        FieldDescriptor descriptor{
            std::string(key),
            CreateDisplayName(key),
            &values,
            GetValueType<T>(),
            inspectorType,
        };
        descriptor.drawAccessor = [](const FieldDescriptor& field)
        {
            DrawList(field, *static_cast<std::vector<T>*>(field.value));
        };
        ConfigureSerialization(descriptor, values);
        m_fields.push_back(std::move(descriptor));
    }

    template <typename T, typename Getter, typename Setter>
    void AddAccessor(
        std::string_view key,
        Getter getter,
        Setter setter,
        InspectorFieldSpace inspectorSpace,
        InspectorFieldType inspectorType = InspectorFieldType::Auto,
        bool serialize = false)
    {
        FieldDescriptor descriptor{
            std::string(key),
            CreateDisplayName(key),
            nullptr,
            GetValueType<T>(),
            inspectorType,
            inspectorSpace,
        };

        descriptor.drawAccessor = [getter, setter](const FieldDescriptor& field)
        {
            T value = getter();
            if (DrawValue(field, &value))
            {
                setter(value);
            }
        };

        if (serialize)
        {
            descriptor.toYAMLValue = [getter](YAMLValue& output, const YAMLValueConversionContext& context)
            {
                return YAMLValueConverter<T>::ToYAMLValue(getter(), output, context);
            };
            descriptor.fromYAMLValue = [setter](
                const YAMLValue& input,
                const YAMLValueConversionContext& context,
                std::function<void()>& assignment)
            {
                T value{};
                YAMLValueConversionResult result = YAMLValueConverter<T>::FromYAMLValue(input, value, context);
                if (result.success)
                {
                    assignment = [setter, value = std::move(value)]() mutable
                    {
                        setter(value);
                    };
                }
                return result;
            };
        }
        m_fields.push_back(std::move(descriptor));
    }

    void DrawImGui(InspectorFieldSpace inspectorSpace = InspectorFieldSpace::All);

    // 登録Field全体をYAMLValueのMapへ変換する。
    bool ToYAMLValue(YAMLValue& output, const YAMLValueConversionContext& context = {}) const;

    // YAMLValueのMapを検証し、全Fieldの変換成功を確認してから、一括で反映
    bool FromYAMLValue(const YAMLValue& input, const YAMLValueConversionContext& context = {});

    const std::vector<FieldDescriptor>& GetFields() const
    {
        return m_fields;
    }
};

#define GM_FIELD(member) fields.Add(#member, member)
#define GM_FIELD_KEY(key, member) fields.Add(key, member)
#define GM_FIELD_KEY_TYPE(key, member, type) fields.Add(key, member, type)
#define GM_FIELD_TYPE(member, type) fields.Add(#member, member, type)
#define GM_FIELD_ACCESSOR(type, key, getter, setter, space) \
    fields.AddAccessor<type>(key, \
        [this] { return (getter); }, \
        [this](const type& value) { setter; }, \
        InspectorFieldSpace::space)
#define GM_FIELD_ACCESSOR_TYPE(type, key, getter, setter, space, inspectorType, serialize) \
    fields.AddAccessor<type>(key, \
        [this] { return (getter); }, \
        [this](const type& value) { setter; }, \
        InspectorFieldSpace::space, inspectorType, serialize)

#include "System/Editor/FieldRegistry.inl"
#include <functional>
