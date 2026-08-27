#pragma once

#include <filesystem>

class ImGuiStyleConfig final
{
public:
    // 標準テーマを基準に、設定ファイルで指定された色とレイアウトだけを上書きする。
    static void Apply(const std::filesystem::path& path);
};
