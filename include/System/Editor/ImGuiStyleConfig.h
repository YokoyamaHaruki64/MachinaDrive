#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

class ImGuiStyleConfig final
{
    static std::filesystem::path s_configPath;
    static std::filesystem::path s_fontPath;
    static std::filesystem::path s_iniPath;
    static std::string s_iniPathString;
    // 設定項目数が少ないため、ハッシュ管理より小さな線形コンテナを使う。
    static std::vector<std::pair<std::string, std::string>> s_styleValues;

public:
    // 設定ファイルを一度読み込み、スタイルと各種パスを保持する。
    static bool Load(const std::filesystem::path& path);

    // 初回読み込みに使った設定ファイルを再読み込みする。
    static bool Reload();

    // 標準テーマを基準に、Load済みのスタイル設定を反映する。
    static void Apply();

    static const std::filesystem::path& GetFontPath() { return s_fontPath; }
    static const std::filesystem::path& GetIniPath() { return s_iniPath; }
    static const std::string& GetIniPathString() { return s_iniPathString; }
};
