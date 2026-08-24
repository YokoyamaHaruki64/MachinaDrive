#include "pch.h"

#define C4_ERROR_THROWS_EXCEPTION
#define RYML_SINGLE_HDR_DEFINE_NOW
#include "External/Include/rapidyaml/rapidyaml.hpp"

#include "System/YAMLArchive.h"

#include "System/FieldRegistry.h"
#include "System/Logger.h"

class YAMLArchive::Implementation final
{
public:
    // 全Fieldの検証完了後にまとめて反映し、途中失敗で既存状態を壊さない。
    using Assignments = std::vector<std::function<void()>>;

    static void WriteValue(ryml::NodeRef node, const YAMLValue& value)
    {
        switch (value.type)
        {
        case YAMLValue::Type::Null:
            node.set_val(c4::csubstr{});
            break;
        case YAMLValue::Type::Scalar:
            if (value.isString)
            {
                node.save(value.scalar);
            }
            else
            {
                node.set_val(c4::to_csubstr(value.scalar));
            }
            break;
        case YAMLValue::Type::Sequence:
            node |= ryml::SEQ;
            for (const YAMLValue& element : value.sequence)
            {
                WriteValue(node.append_child(), element);
            }
            break;
        case YAMLValue::Type::Map:
            node |= ryml::MAP;
            for (const auto& [key, childValue] : value.map)
            {
                WriteValue(node[c4::to_csubstr(key)], childValue);
            }
            break;
        }
    }

    static bool ReadValue(const ryml::ConstNodeRef& node, YAMLValue& value)
    {
        if (node.is_map())
        {
            value = YAMLValue::Map();
            value.map.reserve(node.num_children());
            for (const ryml::ConstNodeRef child : node.children())
            {
                YAMLValue childValue;
                if (!ReadValue(child, childValue))
                {
                    return false;
                }
                value.map.emplace_back(
                    std::string(child.key().str, child.key().len),
                    std::move(childValue));
            }
            return true;
        }

        if (node.is_seq())
        {
            value = YAMLValue::Sequence();
            value.sequence.reserve(node.num_children());
            for (const ryml::ConstNodeRef child : node.children())
            {
                YAMLValue childValue;
                if (!ReadValue(child, childValue))
                {
                    return false;
                }
                value.sequence.emplace_back(std::move(childValue));
            }
            return true;
        }

        if (node.val_is_null())
        {
            value = YAMLValue::Null();
            return true;
        }
        if (!node.has_val())
        {
            return false;
        }

        value = YAMLValue::Scalar(std::string(node.val().str, node.val().len));
        return true;
    }

    static void ReportCodecMessage(const YAMLCodecResult& result, const std::string& fieldKey)
    {
        if (result.message.empty())
        {
            return;
        }

        const std::string message = result.message + ": " + fieldKey;
        if (result.messageLevel == YAMLMessageLevel::Error)
        {
            Logger::Error(message);
        }
        else if (result.messageLevel == YAMLMessageLevel::Warning)
        {
            Logger::Warning(message);
        }
    }

    static const YAMLValue* FindMapValue(const YAMLValue& map, const std::string& key)
    {
        const auto found = std::find_if(map.map.begin(), map.map.end(), [&key](const auto& entry)
        {
            return entry.first == key;
        });
        return found == map.map.end() ? nullptr : &found->second;
    }
};

bool YAMLArchive::SaveFields(
    const std::filesystem::path& path,
    const FieldRegistry& fields,
    const YAMLArchiveContext& context)
{
    try
    {
        YAMLValue document = YAMLValue::Map();
        for (const FieldDescriptor& field : fields.GetFields())
        {
            if (!field.save)
            {
                Logger::Warning("YAML保存対象外のFieldをスキップしました: " + field.key);
                continue;
            }

            YAMLValue value;
            const YAMLCodecResult result = field.save(value, context);
            Implementation::ReportCodecMessage(result, field.key);
            if (!result.success)
            {
                Logger::Warning("YAML保存対象外のFieldをスキップしました: " + field.key);
                continue;
            }
            document.map.emplace_back(field.key, std::move(value));
        }

        ryml::Tree tree;
        Implementation::WriteValue(tree.rootref(), document);

        const std::filesystem::path parent = path.parent_path();
        if (!parent.empty())
        {
            std::filesystem::create_directories(parent);
        }

        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream)
        {
            Logger::Error("YAMLファイルを開けません: " + path.string());
            return false;
        }

        stream << tree;
        if (!stream)
        {
            Logger::Error("YAMLファイルの書き込みに失敗しました: " + path.string());
            return false;
        }
        return true;
    }
    catch (const std::exception& exception)
    {
        Logger::Error("YAML保存に失敗しました: " + std::string(exception.what()));
        return false;
    }
}

bool YAMLArchive::LoadFields(
    const std::filesystem::path& path,
    FieldRegistry& fields,
    const YAMLArchiveContext& context)
{
    try
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream)
        {
            Logger::Error("YAMLファイルを開けません: " + path.string());
            return false;
        }

        std::string source((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        ryml::Tree tree = ryml::parse_in_arena(c4::to_csubstr(source));

        YAMLValue document;
        if (!Implementation::ReadValue(tree.crootref(), document) || document.type != YAMLValue::Type::Map)
        {
            Logger::Error("YAMLルートはMapである必要があります: " + path.string());
            return false;
        }

        Implementation::Assignments assignments;
        for (const FieldDescriptor& field : fields.GetFields())
        {
            const YAMLValue* value = Implementation::FindMapValue(document, field.key);
            if (!value)
            {
                continue;
            }
            if (!field.load)
            {
                Logger::Error("YAML Fieldの読み込みに失敗しました: " + field.key);
                return false;
            }

            std::function<void()> assignment;
            const YAMLCodecResult result = field.load(*value, context, assignment);
            Implementation::ReportCodecMessage(result, field.key);
            if (!result.success || !assignment)
            {
                Logger::Error("YAML Fieldの読み込みに失敗しました: " + field.key);
                return false;
            }
            assignments.emplace_back(std::move(assignment));
        }

        for (const auto& [key, value] : document.map)
        {
            const auto found = std::find_if(fields.GetFields().begin(), fields.GetFields().end(), [&key](const FieldDescriptor& field)
            {
                return field.key == key;
            });
            if (found == fields.GetFields().end())
            {
                Logger::Error("未登録のYAML Fieldをスキップしました: " + key);
            }
        }

        for (const std::function<void()>& assignment : assignments)
        {
            assignment();
        }
        return true;
    }
    catch (const std::exception& exception)
    {
        Logger::Error("YAML読み込みに失敗しました: " + std::string(exception.what()));
        return false;
    }
}
