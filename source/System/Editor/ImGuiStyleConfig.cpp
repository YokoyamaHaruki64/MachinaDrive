#include "pch.h"

#include "System/Editor/ImGuiStyleConfig.h"
#include "System/Diagnostics/Logger.h"

#include "imgui.h"

#include <string_view>

namespace
{
    // 文字列の前後の空白を削除する。
    std::string Trim(std::string_view text)
    {
        // 文字列の先頭の空白文字を見つける
        const size_t first = text.find_first_not_of(" \t\r\n");

        if (first == std::string_view::npos)
        {
            return {};
        }

        // 文字列の末尾の空白文字を見つける
        const size_t last = text.find_last_not_of(" \t\r\n");
        return std::string(text.substr(first, last - first + 1));
    }

    std::string RemoveComment(std::string_view line)
    {
        bool inString = false;
        for (size_t index = 0; index < line.size(); ++index)
        {
            if (line[index] == '"')
            {
                inString = !inString;
            }
            else if (line[index] == '#' && !inString)
            {
                return Trim(line.substr(0, index));
            }
        }

        return Trim(line);
    }

    bool TryParseString(std::string_view value, std::string& result)
    {
        const std::string trimmed = Trim(value);
        if (trimmed.size() < 2 || trimmed.front() != '"' || trimmed.back() != '"')
        {
            return false;
        }

        result = trimmed.substr(1, trimmed.size() - 2);
        return true;
    }

    bool TryParseFloat(std::string_view value, float& result)
    {
        const std::string trimmed = Trim(value);
        try
        {
            size_t parsedLength = 0;
            result = std::stof(trimmed, &parsedLength);
            return parsedLength == trimmed.size();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool TryParseVec2(std::string_view value, ImVec2& result)
    {
        const std::string trimmed = Trim(value);
        if (trimmed.size() < 5 || trimmed.front() != '[' || trimmed.back() != ']')
        {
            return false;
        }

        const std::string_view elements(trimmed.data() + 1, trimmed.size() - 2);
        const size_t separator = elements.find(',');
        if (separator == std::string_view::npos)
        {
            return false;
        }

        float x = 0.0f;
        float y = 0.0f;
        if (!TryParseFloat(elements.substr(0, separator), x) ||
            !TryParseFloat(elements.substr(separator + 1), y))
        {
            return false;
        }

        result = ImVec2(x, y);
        return true;
    }

    bool TryParseHexColor(std::string_view value, ImVec4& result)
    {
        std::string color;
        if (!TryParseString(value, color) || (color.size() != 7 && color.size() != 9) || color.front() != '#')
        {
            return false;
        }

        const auto hexToByte = [](char high, char low, unsigned char& output)
        {
            const auto toNibble = [](char character)
            {
                if (character >= '0' && character <= '9')
                {
                    return character - '0';
                }
                if (character >= 'a' && character <= 'f')
                {
                    return character - 'a' + 10;
                }
                if (character >= 'A' && character <= 'F')
                {
                    return character - 'A' + 10;
                }
                return -1;
            };

            const int highNibble = toNibble(high);
            const int lowNibble = toNibble(low);
            if (highNibble < 0 || lowNibble < 0)
            {
                return false;
            }

            output = static_cast<unsigned char>((highNibble << 4) | lowNibble);
            return true;
        };

        unsigned char red = 0;
        unsigned char green = 0;
        unsigned char blue = 0;
        unsigned char alpha = 255;
        if (!hexToByte(color[1], color[2], red) ||
            !hexToByte(color[3], color[4], green) ||
            !hexToByte(color[5], color[6], blue) ||
            (color.size() == 9 && !hexToByte(color[7], color[8], alpha)))
        {
            return false;
        }

        constexpr float byteToFloat = 1.0f / 255.0f;
        result = ImVec4(red * byteToFloat, green * byteToFloat, blue * byteToFloat, alpha * byteToFloat);
        return true;
    }

    bool ApplyStyleValue(const std::string& key, std::string_view value)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        for (int colorIndex = 0; colorIndex < ImGuiCol_COUNT; ++colorIndex)
        {
            if (key == ImGui::GetStyleColorName(colorIndex))
            {
                return TryParseHexColor(value, style.Colors[colorIndex]);
            }
        }

        if (key == "WindowRounding") return TryParseFloat(value, style.WindowRounding);
        if (key == "FrameRounding") return TryParseFloat(value, style.FrameRounding);
        if (key == "PopupRounding") return TryParseFloat(value, style.PopupRounding);
        if (key == "ScrollbarRounding") return TryParseFloat(value, style.ScrollbarRounding);
        if (key == "TabRounding") return TryParseFloat(value, style.TabRounding);
        if (key == "TabBorderSize") return TryParseFloat(value, style.TabBorderSize);
        if (key == "TabBarBorderSize") return TryParseFloat(value, style.TabBarBorderSize);
        if (key == "TabBarOverlineSize") return TryParseFloat(value, style.TabBarOverlineSize);
        if (key == "WindowBorderSize") return TryParseFloat(value, style.WindowBorderSize);
        if (key == "FrameBorderSize") return TryParseFloat(value, style.FrameBorderSize);
        if (key == "WindowMinSize") return TryParseVec2(value, style.WindowMinSize);
        if (key == "ItemSpacing") return TryParseVec2(value, style.ItemSpacing);
        if (key == "WindowPadding") return TryParseVec2(value, style.WindowPadding);

        return false;
    }

    std::filesystem::path ToUtf8Path(std::string_view value)
    {
        const auto* begin = reinterpret_cast<const char8_t*>(value.data());
        const std::u8string utf8Value(begin, begin + value.size());
        return std::filesystem::path(utf8Value);
    }
}

std::filesystem::path ImGuiStyleConfig::s_configPath = "config/imgui_style.toml";
std::filesystem::path ImGuiStyleConfig::s_fontPath = "config/NotoSansJP-Medium.ttf";
std::filesystem::path ImGuiStyleConfig::s_iniPath = "config/imgui.ini";
std::string ImGuiStyleConfig::s_iniPathString = "config/imgui.ini";
std::vector<std::pair<std::string, std::string>> ImGuiStyleConfig::s_styleValues;

bool ImGuiStyleConfig::Load(const std::filesystem::path& path)
{
    s_configPath = path;
    s_fontPath = "config/NotoSansJP-Medium.ttf";
    s_iniPath = "config/imgui.ini";
    s_iniPathString = "config/imgui.ini";
    s_styleValues.clear();

    std::ifstream stream(path);
    if (!stream)
    {
        Logger::Warning("ImGuiスタイル設定が見つかりません。既定テーマを使用します: " + path.string());
        return false;
    }

    std::string line;
    size_t lineNumber = 0;
    while (std::getline(stream, line))
    {
        ++lineNumber;
        // UTF-8 BOMを削除する
        if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF"))
        {
            line.erase(0, 3);
        }

        // コメント行と空行をスキップする
        const std::string content = RemoveComment(line);
        if (content.empty())
        {
            continue;
        }

        // キーと値を分割する
        const size_t separator = content.find('=');
        if (separator == std::string::npos)
        {
            Logger::Warning("ImGuiスタイル設定の形式が不正です: " + std::to_string(lineNumber));
            continue;
        }

        // キーと値をトリムして設定オブジェクトへ保持する。
        const std::string key = Trim(std::string_view(content).substr(0, separator));
        const std::string value = Trim(std::string_view(content).substr(separator + 1));
        if (key.empty())
        {
            Logger::Warning("ImGuiスタイル設定のキーが空です (" + std::to_string(lineNumber) + ")");
            continue;
        }

        if (key == "FontPath" || key == "IniPath")
        {
            std::string parsedPath;
            if (!TryParseString(value, parsedPath) || parsedPath.empty())
            {
                Logger::Warning("ImGuiパス設定を解析できません: " + key + " (" + std::to_string(lineNumber) + ")");
                continue;
            }

            if (key == "FontPath")
            {
                s_fontPath = ToUtf8Path(parsedPath);
            }
            else
            {
                s_iniPath = ToUtf8Path(parsedPath);
                // ImGuiはUTF-8のファイル名を受け取るため、元のUTF-8表現を保持する。
                s_iniPathString = parsedPath;
            }
            continue;
        }

        // 重複キーも記述順に保持し、Apply時に後の値で上書きする。
        s_styleValues.emplace_back(key, value);
    }

    return true;
}

bool ImGuiStyleConfig::Reload()
{
    return Load(s_configPath);
}

void ImGuiStyleConfig::Apply()
{
    ImGui::StyleColorsDark();

    for (const auto& [key, value] : s_styleValues)
    {
        if (!ApplyStyleValue(key, value))
        {
            Logger::Warning("ImGuiスタイル設定を適用できません: " + key);
        }
    }
}
