#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <limits>
#include <string>
#include <type_traits>
#include <system_error>
#include <vector>

#include "Common/Color.h"
#include "math/Matrix3x3.h"
#include "math/Matrix4x4.h"
#include "math/Curve.h"
#include "math/Quaternion.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "System/GameObjectReference.h"
#include "System/PrefabReference.h"
#include "System/YAMLTypes.h"

template <typename T, typename Enable = void>
struct YAMLCodec;

template <typename T>
struct YAMLFloatSequenceTraits;

class YAMLCodecUtility final
{
public:
    static void MergeMessage(YAMLCodecResult& destination, const YAMLCodecResult& source)
    {
        if (source.message.empty())
        {
            return;
        }

        if (!destination.message.empty())
        {
            destination.message += "; ";
        }
        destination.message += source.message;
        if (static_cast<int>(source.messageLevel) > static_cast<int>(destination.messageLevel))
        {
            destination.messageLevel = source.messageLevel;
        }
    }

    template <typename T>
    static YAMLCodecResult EncodeNumber(T value, YAMLValue& output)
    {
        std::array<char, 64> buffer{};
        std::to_chars_result result;
        if constexpr (std::is_floating_point_v<T>)
        {
            result = std::to_chars(
                buffer.data(),
                buffer.data() + buffer.size(),
                value,
                std::chars_format::general,
                std::numeric_limits<T>::max_digits10);
        }
        else
        {
            result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
        }

        if (result.ec != std::errc{})
        {
            return { false };
        }

        output = YAMLValue::Scalar(std::string(buffer.data(), result.ptr));
        return {};
    }

    template <typename T>
    static YAMLCodecResult DecodeNumber(const YAMLValue& input, T& output)
    {
        if (input.type != YAMLValue::Type::Scalar)
        {
            return { false };
        }

        const char* begin = input.scalar.data();
        const char* end = begin + input.scalar.size();
        const std::from_chars_result result = std::from_chars(begin, end, output);
        return { result.ec == std::errc{} && result.ptr == end };
    }

    template <typename T, size_t Count>
    static YAMLCodecResult EncodeFloatSequence(const T& value, YAMLValue& output)
    {
        const std::array<float, Count> components = ::YAMLFloatSequenceTraits<T>::ToArray(value);
        output = YAMLValue::Sequence(YAMLValue::CollectionStyle::Flow);
        output.sequence.reserve(Count);

        for (float component : components)
        {
            YAMLValue element;
            const YAMLCodecResult result = EncodeNumber(component, element);
            if (!result.success)
            {
                return result;
            }
            output.sequence.emplace_back(std::move(element));
        }
        return {};
    }

    template <typename T, size_t Count>
    static YAMLCodecResult DecodeFloatSequence(const YAMLValue& input, T& output)
    {
        if (input.type != YAMLValue::Type::Sequence || input.sequence.size() != Count)
        {
            return { false };
        }

        std::array<float, Count> components{};
        for (size_t index = 0; index < Count; ++index)
        {
            const YAMLCodecResult result = DecodeNumber(input.sequence[index], components[index]);
            if (!result.success)
            {
                return result;
            }
        }

        output = ::YAMLFloatSequenceTraits<T>::FromArray(components);
        return {};
    }
};

template <typename T>
struct YAMLCodec<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
{
    static YAMLCodecResult Encode(T value, YAMLValue& output, const YAMLArchiveContext&)
    {
        return YAMLCodecUtility::EncodeNumber(value, output);
    }

    static YAMLCodecResult Decode(const YAMLValue& input, T& output, const YAMLArchiveContext&)
    {
        return YAMLCodecUtility::DecodeNumber(input, output);
    }
};

template <typename T>
struct YAMLCodec<T, std::enable_if_t<std::is_floating_point_v<T>>>
{
    static YAMLCodecResult Encode(T value, YAMLValue& output, const YAMLArchiveContext&)
    {
        return YAMLCodecUtility::EncodeNumber(value, output);
    }

    static YAMLCodecResult Decode(const YAMLValue& input, T& output, const YAMLArchiveContext&)
    {
        return YAMLCodecUtility::DecodeNumber(input, output);
    }
};

template <>
struct YAMLCodec<bool>
{
    static YAMLCodecResult Encode(bool value, YAMLValue& output, const YAMLArchiveContext&)
    {
        output = YAMLValue::Scalar(value ? "true" : "false");
        return {};
    }

    static YAMLCodecResult Decode(const YAMLValue& input, bool& output, const YAMLArchiveContext&)
    {
        if (input.type != YAMLValue::Type::Scalar || (input.scalar != "true" && input.scalar != "false"))
        {
            return { false };
        }
        output = input.scalar == "true";
        return {};
    }
};

template <>
struct YAMLCodec<std::string>
{
    static YAMLCodecResult Encode(const std::string& value, YAMLValue& output, const YAMLArchiveContext&)
    {
        output = YAMLValue::Scalar(value, true);
        return {};
    }

    static YAMLCodecResult Decode(const YAMLValue& input, std::string& output, const YAMLArchiveContext&)
    {
        if (input.type != YAMLValue::Type::Scalar)
        {
            return { false };
        }
        output = input.scalar;
        return {};
    }
};

template <>
struct YAMLFloatSequenceTraits<Vector2>
{
    static constexpr size_t Count = 2;

    static std::array<float, Count> ToArray(const Vector2& value)
    {
        return { value.x, value.y };
    }

    static Vector2 FromArray(const std::array<float, Count>& values)
    {
        return { values[0], values[1] };
    }
};

template <>
struct YAMLFloatSequenceTraits<Vector3>
{
    static constexpr size_t Count = 3;

    static std::array<float, Count> ToArray(const Vector3& value)
    {
        return { value.x, value.y, value.z };
    }

    static Vector3 FromArray(const std::array<float, Count>& values)
    {
        return { values[0], values[1], values[2] };
    }
};

template <>
struct YAMLFloatSequenceTraits<Vector4>
{
    static constexpr size_t Count = 4;

    static std::array<float, Count> ToArray(const Vector4& value)
    {
        return { value.x, value.y, value.z, value.w };
    }

    static Vector4 FromArray(const std::array<float, Count>& values)
    {
        return { values[0], values[1], values[2], values[3] };
    }
};

template <>
struct YAMLFloatSequenceTraits<Color>
{
    static constexpr size_t Count = 4;

    static std::array<float, Count> ToArray(const Color& value)
    {
        return { value.r, value.g, value.b, value.a };
    }

    static Color FromArray(const std::array<float, Count>& values)
    {
        return { values[0], values[1], values[2], values[3] };
    }
};

template <>
struct YAMLFloatSequenceTraits<Quaternion>
{
    static constexpr size_t Count = 4;

    static std::array<float, Count> ToArray(const Quaternion& value)
    {
        return { value.x, value.y, value.z, value.w };
    }

    static Quaternion FromArray(const std::array<float, Count>& values)
    {
        return Quaternion(values[0], values[1], values[2], values[3]).Normalized();
    }
};

template <>
struct YAMLFloatSequenceTraits<Matrix3x3>
{
    static constexpr size_t Count = 9;
    static std::array<float, Count> ToArray(const Matrix3x3& value)
    {
        std::array<float, Count> result{};
        std::copy(&value.m[0][0], &value.m[0][0] + result.size(), result.begin());
        return result;
    }

    static Matrix3x3 FromArray(const std::array<float, Count>& values)
    {
        Matrix3x3 result;
        std::copy(values.begin(), values.end(), &result.m[0][0]);
        return result;
    }
};

template <>
struct YAMLFloatSequenceTraits<Matrix4x4>
{
    static constexpr size_t Count = 16;
    static std::array<float, Count> ToArray(const Matrix4x4& value)
    {
        std::array<float, Count> result{};
        std::copy(&value.m[0][0], &value.m[0][0] + result.size(), result.begin());
        return result;
    }

    static Matrix4x4 FromArray(const std::array<float, Count>& values)
    {
        Matrix4x4 result;
        std::copy(values.begin(), values.end(), &result.m[0][0]);
        return result;
    }
};

template <typename T>
struct YAMLCodec<T, std::void_t<decltype(YAMLFloatSequenceTraits<T>::Count)>>
{
    static YAMLCodecResult Encode(const T& value, YAMLValue& output, const YAMLArchiveContext&)
    {
        return YAMLCodecUtility::EncodeFloatSequence<T, YAMLFloatSequenceTraits<T>::Count>(value, output);
    }

    static YAMLCodecResult Decode(const YAMLValue& input, T& output, const YAMLArchiveContext&)
    {
        return YAMLCodecUtility::DecodeFloatSequence<T, YAMLFloatSequenceTraits<T>::Count>(input, output);
    }
};

template <typename T>
struct YAMLCodec<Curve<T>>
{
    static YAMLCodecResult Encode(
        const Curve<T>& curve,
        YAMLValue& output,
        const YAMLArchiveContext& context)
    {
        output = YAMLValue::Sequence();
        output.sequence.reserve(curve.GetKeyCount());
        for (const typename Curve<T>::Key& key : curve.GetKeys())
        {
            YAMLValue encodedKey = YAMLValue::Map();
            YAMLValue time;
            YAMLValue value;
            YAMLValue inControl;
            YAMLValue outControl;
            if (!YAMLCodec<float>::Encode(key.time, time, context).success ||
                !YAMLCodec<T>::Encode(key.value, value, context).success ||
                !YAMLCodec<T>::Encode(key.inControl, inControl, context).success ||
                !YAMLCodec<T>::Encode(key.outControl, outControl, context).success)
            {
                return { false };
            }

            encodedKey.Add("time", std::move(time));
            encodedKey.Add("value", std::move(value));
            encodedKey.Add("inControl", std::move(inControl));
            encodedKey.Add("outControl", std::move(outControl));
            output.sequence.emplace_back(std::move(encodedKey));
        }
        return {};
    }

    static YAMLCodecResult Decode(
        const YAMLValue& input,
        Curve<T>& output,
        const YAMLArchiveContext& context)
    {
        if (input.type != YAMLValue::Type::Sequence)
        {
            return { false };
        }

        std::vector<typename Curve<T>::Key> keys;
        keys.reserve(input.sequence.size());
        for (const YAMLValue& encodedKey : input.sequence)
        {
            if (encodedKey.type != YAMLValue::Type::Map)
            {
                return { false };
            }

            const YAMLValue* time = encodedKey.Find("time");
            const YAMLValue* value = encodedKey.Find("value");
            const YAMLValue* inControl = encodedKey.Find("inControl");
            const YAMLValue* outControl = encodedKey.Find("outControl");
            typename Curve<T>::Key key;
            if (!time || !value || !inControl || !outControl ||
                !YAMLCodec<float>::Decode(*time, key.time, context).success ||
                !YAMLCodec<T>::Decode(*value, key.value, context).success ||
                !YAMLCodec<T>::Decode(*inControl, key.inControl, context).success ||
                !YAMLCodec<T>::Decode(*outControl, key.outControl, context).success)
            {
                return { false };
            }
            keys.emplace_back(std::move(key));
        }

        output.SetKeys(keys);
        return {};
    }
};

template <>
struct YAMLCodec<CurveColor>
{
    static YAMLCodecResult Encode(
        const CurveColor& curve,
        YAMLValue& output,
        const YAMLArchiveContext& context)
    {
        YAMLValue red;
        YAMLValue green;
        YAMLValue blue;
        YAMLValue alpha;
        if (!YAMLCodec<Curve<float>>::Encode(curve.r, red, context).success ||
            !YAMLCodec<Curve<float>>::Encode(curve.g, green, context).success ||
            !YAMLCodec<Curve<float>>::Encode(curve.b, blue, context).success ||
            !YAMLCodec<Curve<float>>::Encode(curve.a, alpha, context).success)
        {
            return { false };
        }

        output = YAMLValue::Map();
        output.Add("r", std::move(red));
        output.Add("g", std::move(green));
        output.Add("b", std::move(blue));
        output.Add("a", std::move(alpha));
        return {};
    }

    static YAMLCodecResult Decode(
        const YAMLValue& input,
        CurveColor& output,
        const YAMLArchiveContext& context)
    {
        if (input.type != YAMLValue::Type::Map)
        {
            return { false };
        }

        const YAMLValue* red = input.Find("r");
        const YAMLValue* green = input.Find("g");
        const YAMLValue* blue = input.Find("b");
        const YAMLValue* alpha = input.Find("a");
        CurveColor decoded;
        if (!red || !green || !blue || !alpha ||
            !YAMLCodec<Curve<float>>::Decode(*red, decoded.r, context).success ||
            !YAMLCodec<Curve<float>>::Decode(*green, decoded.g, context).success ||
            !YAMLCodec<Curve<float>>::Decode(*blue, decoded.b, context).success ||
            !YAMLCodec<Curve<float>>::Decode(*alpha, decoded.a, context).success)
        {
            return { false };
        }

        output = std::move(decoded);
        return {};
    }
};

template <>
struct YAMLCodec<PrefabReference>
{
    static YAMLCodecResult Encode(const PrefabReference& value, YAMLValue& output, const YAMLArchiveContext&)
    {
        output = value ? YAMLValue::Scalar(value.GetPath(), true) : YAMLValue::Null();
        return {};
    }

    static YAMLCodecResult Decode(
        const YAMLValue& input,
        PrefabReference& output,
        const YAMLArchiveContext& context)
    {
        if (input.type == YAMLValue::Type::Null)
        {
            output.Clear();
            return {};
        }
        if (input.type != YAMLValue::Type::Scalar)
        {
            return { false };
        }

        if (context.resolvePrefab && !context.resolvePrefab(input.scalar))
        {
            output.MarkResolutionError(input.scalar);
            return { true, YAMLMessageLevel::Error, "Prefab参照を解決できません" };
        }

        output.SetPath(input.scalar);
        return {};
    }
};

template <>
struct YAMLCodec<GameObjectReference>
{
    static YAMLCodecResult Encode(
        const GameObjectReference& value,
        YAMLValue& output,
        const YAMLArchiveContext& context)
    {
        if (!value)
        {
            output = YAMLValue::Null();
            return {};
        }
        if (!context.getObjectId)
        {
            output = YAMLValue::Null();
            return { true, YAMLMessageLevel::Warning, "GameObject参照のID変換が未指定です" };
        }

        const std::optional<YAMLArchiveContext::ObjectId> id = context.getObjectId(value.Get());
        if (!id)
        {
            output = YAMLValue::Null();
            return { true, YAMLMessageLevel::Warning, "保存範囲外のGameObject参照をNoneとして保存します" };
        }
        return YAMLCodec<YAMLArchiveContext::ObjectId>::Encode(*id, output, context);
    }

    static YAMLCodecResult Decode(
        const YAMLValue& input,
        GameObjectReference& output,
        const YAMLArchiveContext& context)
    {
        if (input.type == YAMLValue::Type::Null)
        {
            output.Clear();
            return {};
        }

        YAMLArchiveContext::ObjectId id{};
        const YAMLCodecResult idResult = YAMLCodec<YAMLArchiveContext::ObjectId>::Decode(input, id, context);
        if (!idResult.success)
        {
            return idResult;
        }

        GameObject* resolved = context.resolveObject ? context.resolveObject(id) : nullptr;
        if (!resolved)
        {
            output.MarkResolutionError();
            return { true, YAMLMessageLevel::Error, "GameObject参照を解決できません" };
        }

        output.Set(resolved);
        return {};
    }
};

template <typename T>
struct YAMLCodec<std::vector<T>>
{
    static YAMLCodecResult Encode(
        const std::vector<T>& values,
        YAMLValue& output,
        const YAMLArchiveContext& context)
    {
        output = YAMLValue::Sequence();
        output.sequence.reserve(values.size());
        YAMLCodecResult aggregate;
        for (const auto& sourceElement : values)
        {
            const T element = sourceElement;
            YAMLValue encoded;
            const YAMLCodecResult result = YAMLCodec<T>::Encode(element, encoded, context);
            if (!result.success)
            {
                return result;
            }
            YAMLCodecUtility::MergeMessage(aggregate, result);
            output.sequence.emplace_back(std::move(encoded));
        }
        return aggregate;
    }

    static YAMLCodecResult Decode(
        const YAMLValue& input,
        std::vector<T>& output,
        const YAMLArchiveContext& context)
    {
        if (input.type != YAMLValue::Type::Sequence)
        {
            return { false };
        }

        std::vector<T> decoded;
        decoded.reserve(input.sequence.size());
        YAMLCodecResult aggregate;
        for (const YAMLValue& sourceElement : input.sequence)
        {
            T element{};
            const YAMLCodecResult result = YAMLCodec<T>::Decode(sourceElement, element, context);
            if (!result.success)
            {
                return result;
            }
            YAMLCodecUtility::MergeMessage(aggregate, result);
            decoded.emplace_back(std::move(element));
        }

        output = std::move(decoded);
        return aggregate;
    }
};
