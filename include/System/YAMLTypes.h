#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class GameObject;

struct YAMLArchiveContext
{
    using ObjectId = uint64_t;

    std::function<std::optional<ObjectId>(const GameObject*)> getObjectId;
    std::function<GameObject*(ObjectId)> resolveObject;
};

enum class YAMLMessageLevel
{
    None,
    Warning,
    Error,
};

struct YAMLCodecResult
{
    bool success = true;
    YAMLMessageLevel messageLevel = YAMLMessageLevel::None;
    std::string message;
};

// YAMLライブラリに依存しない中間表現。Codecとファイル形式の実装を分離する。
struct YAMLValue
{
    enum class Type
    {
        Null,
        Scalar,
        Sequence,
        Map,
    };

    Type type = Type::Null;
    std::string scalar;
    bool isString = false;
    std::vector<YAMLValue> sequence;
    std::vector<std::pair<std::string, YAMLValue>> map;

    static YAMLValue Null()
    {
        return {};
    }

    static YAMLValue Scalar(std::string value, bool stringValue = false)
    {
        YAMLValue result;
        result.type = Type::Scalar;
        result.scalar = std::move(value);
        result.isString = stringValue;
        return result;
    }

    static YAMLValue Sequence()
    {
        YAMLValue result;
        result.type = Type::Sequence;
        return result;
    }

    static YAMLValue Map()
    {
        YAMLValue result;
        result.type = Type::Map;
        return result;
    }

    YAMLValue& Add(std::string key, YAMLValue value)
    {
        map.emplace_back(std::move(key), std::move(value));
        return map.back().second;
    }

    const YAMLValue* Find(std::string_view key) const
    {
        for (const auto& [entryKey, value] : map)
        {
            if (entryKey == key)
            {
                return &value;
            }
        }
        return nullptr;
    }
};
