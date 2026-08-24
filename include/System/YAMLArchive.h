#pragma once

#include <filesystem>

#include "System/YAMLTypes.h"

class FieldRegistry;

// YAMLライブラリの詳細を隠し、登録済みFieldの入出力を担当する。
class YAMLArchive final
{
public:
    static bool SaveDocument(const std::filesystem::path& path, const YAMLValue& document);
    static bool LoadDocument(const std::filesystem::path& path, YAMLValue& document);

    static bool EncodeFields(
        const FieldRegistry& fields,
        YAMLValue& output,
        const YAMLArchiveContext& context = {});
    static bool DecodeFields(
        const YAMLValue& input,
        FieldRegistry& fields,
        const YAMLArchiveContext& context = {});

    static bool SaveFields(
        const std::filesystem::path& path,
        const FieldRegistry& fields,
        const YAMLArchiveContext& context = {});
    static bool LoadFields(
        const std::filesystem::path& path,
        FieldRegistry& fields,
        const YAMLArchiveContext& context = {});

private:
    class Implementation;
};
