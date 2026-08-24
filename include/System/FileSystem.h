#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

// OS依存のダイアログと、プロジェクト内で共用するファイルパス操作を提供する。
class FileSystem final
{
public:
    FileSystem() = delete;

    static bool Exists(const std::filesystem::path& path);
    static bool CreateDirectories(const std::filesystem::path& path);
    static std::filesystem::path Normalize(const std::filesystem::path& path);
    static std::optional<std::filesystem::path> ToProjectRelative(const std::filesystem::path& path);
    static bool IsUnderDirectory(
        const std::filesystem::path& path,
        const std::filesystem::path& directory);

    static std::vector<std::filesystem::path> FindFiles(
        const std::filesystem::path& directory,
        std::string_view extension,
        bool recursive = true);

    static std::optional<std::filesystem::path> OpenFileDialog(
        const std::filesystem::path& initialDirectory,
        std::string_view extension);
    static std::optional<std::filesystem::path> OpenFileDialog(
        const std::filesystem::path& initialDirectory,
        std::span<const std::string_view> extensions);
    static std::optional<std::filesystem::path> SaveFileDialog(
        const std::filesystem::path& initialDirectory,
        std::string_view extension,
        std::string_view defaultFileName = {});

private:
    class Implementation;
};
