#include "pch.h"

#include "System/Editor/FileSystem.h"

#include <array>
#include <commdlg.h>

#include "System/Diagnostics/Logger.h"
#include "System/Core/main.h"

std::wstring FileSystem::NormalizeExtension(std::string_view extension)
{
    if (extension.empty())
    {
        return {};
    }

    std::string normalized(extension);
    if (normalized.front() != '.')
    {
        normalized.insert(normalized.begin(), '.');
    }
    return std::filesystem::path(normalized).wstring();
}

std::vector<std::wstring> FileSystem::NormalizeExtensions(std::span<const std::string_view> extensions)
{
    std::vector<std::wstring> normalizedExtensions;
    normalizedExtensions.reserve(extensions.size());
    for (std::string_view extension : extensions)
    {
        std::wstring normalizedExtension = NormalizeExtension(extension);
        if (!normalizedExtension.empty())
        {
            normalizedExtensions.emplace_back(std::move(normalizedExtension));
        }
    }
    return normalizedExtensions;
}

std::wstring FileSystem::CreateFilter(std::span<const std::wstring> extensions)
{
    // Win32のフィルタは終端に追加のNUL文字を要求する。
    std::wstring pattern;
    for (const std::wstring& extension : extensions)
    {
        if (!pattern.empty())
        {
            pattern += L';';
        }
        pattern += L'*' + extension;
    }

    std::wstring filter = L"Supported Files (" + pattern + L")";
    filter.push_back(L'\0');
    filter += pattern;
    filter.push_back(L'\0');
    filter.push_back(L'\0');
    return filter;
}

bool FileSystem::HasExtension(
    const std::filesystem::path& path,
    std::span<const std::wstring> extensions)
{
    return std::ranges::any_of(extensions, [&path](const std::wstring& extension)
    {
        return _wcsicmp(path.extension().c_str(), extension.c_str()) == 0;
    });
}

bool FileSystem::HasExtension(const std::filesystem::path& path, const std::wstring& extension)
{
    return !extension.empty() && _wcsicmp(path.extension().c_str(), extension.c_str()) == 0;
}

std::optional<std::filesystem::path> FileSystem::ShowFileDialog(
    bool saveFile,
    const std::filesystem::path& initialDirectory,
    std::span<const std::string_view> extensions,
    std::string_view defaultFileName)
{
    // 相対パスを渡しても、ダイアログの初期位置を実行環境に依存させない。
    std::error_code error;
    const std::filesystem::path absoluteDirectory = std::filesystem::absolute(initialDirectory, error);
    if (error || !std::filesystem::is_directory(absoluteDirectory, error))
    {
        Logger::Error("ファイルダイアログの初期フォルダを開けません: " + initialDirectory.string());
        return std::nullopt;
    }

    const std::vector<std::wstring> normalizedExtensions = NormalizeExtensions(extensions);
    if (normalizedExtensions.empty())
    {
        Logger::Error("ファイルダイアログには拡張子の指定が必要です。");
        return std::nullopt;
    }

    std::array<wchar_t, MaxDialogPathLength> filePath{};
    const std::wstring wideDefaultName = std::filesystem::path(std::string(defaultFileName)).wstring();
    if (wideDefaultName.size() >= filePath.size())
    {
        Logger::Error("ファイルダイアログの既定ファイル名が長すぎます。");
        return std::nullopt;
    }
    std::copy(wideDefaultName.begin(), wideDefaultName.end(), filePath.begin());

    const std::wstring filter = CreateFilter(normalizedExtensions);
    const std::wstring wideInitialDirectory = absoluteDirectory.wstring();
    const std::wstring defaultExtension = normalizedExtensions.front().substr(1);

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetWindow();
    dialog.lpstrFilter = filter.c_str();
    dialog.lpstrFile = filePath.data();
    dialog.nMaxFile = static_cast<DWORD>(filePath.size());
    dialog.lpstrInitialDir = wideInitialDirectory.c_str();
    dialog.lpstrDefExt = defaultExtension.c_str();
    dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;
    dialog.Flags |= saveFile ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST;

    const BOOL selected = saveFile
        ? GetSaveFileNameW(&dialog)
        : GetOpenFileNameW(&dialog);
    if (!selected)
    {
        const DWORD dialogError = CommDlgExtendedError();
        if (dialogError != 0)
        {
            Logger::Error("ファイルダイアログの表示に失敗しました: " + std::to_string(dialogError));
        }
        return std::nullopt;
    }

    // 選択後も拡張子を再検証し、ダイアログAPIの既定拡張子補完に依存しない。
    const std::filesystem::path selectedPath = Normalize(filePath.data());
    if (!HasExtension(selectedPath, normalizedExtensions))
    {
        Logger::Error("選択したファイルの拡張子が一致しません: " + selectedPath.string());
        return std::nullopt;
    }
    return selectedPath;
}

bool FileSystem::Exists(const std::filesystem::path& path)
{
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

bool FileSystem::CreateDirectories(const std::filesystem::path& path)
{
    std::error_code error;
    if (std::filesystem::is_directory(path, error))
    {
        return true;
    }
    error.clear();
    return std::filesystem::create_directories(path, error) && !error;
}

std::filesystem::path FileSystem::Normalize(const std::filesystem::path& path)
{
    return path.lexically_normal();
}

std::optional<std::filesystem::path> FileSystem::ToProjectRelative(const std::filesystem::path& path)
{
    std::error_code error;
    const std::filesystem::path projectRoot = std::filesystem::current_path(error);
    if (error || !IsUnderDirectory(path, projectRoot))
    {
        return std::nullopt;
    }

    const std::filesystem::path relativePath = std::filesystem::relative(path, projectRoot, error);
    return error ? std::nullopt : std::optional<std::filesystem::path>(Normalize(relativePath));
}

bool FileSystem::IsUnderDirectory(
    const std::filesystem::path& path,
    const std::filesystem::path& directory)
{
    std::error_code error;
    const std::filesystem::path absolutePath = std::filesystem::absolute(path, error).lexically_normal();
    if (error)
    {
        return false;
    }
    const std::filesystem::path absoluteDirectory = std::filesystem::absolute(directory, error).lexically_normal();
    if (error)
    {
        return false;
    }

    const std::filesystem::path relativePath = std::filesystem::relative(absolutePath, absoluteDirectory, error);
    return !error && !relativePath.empty() && *relativePath.begin() != "..";
}

std::vector<std::filesystem::path> FileSystem::FindFiles(
    const std::filesystem::path& directory,
    std::string_view extension,
    bool recursive)
{
    std::vector<std::filesystem::path> result;
    const std::wstring normalizedExtension = NormalizeExtension(extension);
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error) || normalizedExtension.empty())
    {
        return result;
    }

    const auto collect = [&result, &normalizedExtension](const std::filesystem::directory_entry& entry)
    {
        if (entry.is_regular_file() && HasExtension(entry.path(), normalizedExtension))
        {
            result.emplace_back(Normalize(entry.path()));
        }
    };

    if (recursive)
    {
        for (std::filesystem::recursive_directory_iterator iterator(
            directory,
            std::filesystem::directory_options::skip_permission_denied,
            error), end;
            iterator != end;
            iterator.increment(error))
        {
            if (!error)
            {
                collect(*iterator);
            }
            else
            {
                Logger::Warning("ファイル探索中にディレクトリを読み取れません: " + directory.string());
                break;
            }
        }
    }
    else
    {
        for (std::filesystem::directory_iterator iterator(
            directory,
            std::filesystem::directory_options::skip_permission_denied,
            error), end;
            iterator != end;
            iterator.increment(error))
        {
            if (!error)
            {
                collect(*iterator);
            }
            else
            {
                Logger::Warning("ファイル探索中にディレクトリを読み取れません: " + directory.string());
                break;
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

std::optional<std::filesystem::path> FileSystem::OpenFileDialog(
    const std::filesystem::path& initialDirectory,
    std::string_view extension)
{
    const std::array extensions{ extension };
    return ShowFileDialog(false, initialDirectory, extensions, {});
}

std::optional<std::filesystem::path> FileSystem::OpenFileDialog(
    const std::filesystem::path& initialDirectory,
    std::span<const std::string_view> extensions)
{
    return ShowFileDialog(false, initialDirectory, extensions, {});
}

std::optional<std::filesystem::path> FileSystem::SaveFileDialog(
    const std::filesystem::path& initialDirectory,
    std::string_view extension,
    std::string_view defaultFileName)
{
    const std::array extensions{ extension };
    return ShowFileDialog(true, initialDirectory, extensions, defaultFileName);
}
