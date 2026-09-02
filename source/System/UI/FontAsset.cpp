#include "pch.h"

#include "System/UI/FontAsset.h"

#include <fstream>
#include <optional>
#include <sstream>

#include "Graphics/TextureManager.h"
#include "System/Diagnostics/Logger.h"

namespace
{
    std::unordered_map<std::string, std::unique_ptr<FontAsset>> g_fontAssets;

    bool ParseInteger(const std::string& token, const char* key, int& output)
    {
        const std::string prefix = std::string(key) + "=";
        if (!token.starts_with(prefix))
        {
            return false;
        }
        output = std::stoi(token.substr(prefix.size()));
        return true;
    }

    std::optional<std::filesystem::path> ParsePageFilePath(const std::string& line)
    {
        constexpr std::string_view key = "file=";
        const size_t keyPosition = line.find(key);
        if (keyPosition == std::string::npos)
        {
            return std::nullopt;
        }

        size_t valueBegin = keyPosition + key.size();
        if (valueBegin >= line.size())
        {
            return std::nullopt;
        }

        if (line[valueBegin] == '"')
        {
            const size_t valueEnd = line.find('"', ++valueBegin);
            if (valueEnd == std::string::npos)
            {
                return std::nullopt;
            }
            return std::filesystem::path(line.substr(valueBegin, valueEnd - valueBegin));
        }

        const size_t valueEnd = line.find_first_of(" \t\r\n", valueBegin);
        return std::filesystem::path(line.substr(valueBegin, valueEnd - valueBegin));
    }
}

const FontGlyph* FontAsset::FindGlyph(uint32_t codePoint) const
{
    const auto found = m_glyphs.find(codePoint);
    return found != m_glyphs.end() ? &found->second : nullptr;
}

bool FontAsset::LoadFromFile(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    if (!stream)
    {
        Logger::Error("FontAssetを開けません: " + path.string());
        return false;
    }

    m_path = path;
    std::filesystem::path texturePath;
    std::string line;
    while (std::getline(stream, line))
    {
        std::istringstream tokens(line);
        std::string type;
        tokens >> type;
        if (type == "page")
        {
            if (const std::optional<std::filesystem::path> pageFile = ParsePageFilePath(line))
            {
                texturePath = path.parent_path() / *pageFile;
            }
        }
        else if (type == "common")
        {
            std::string token;
            while (tokens >> token)
            {
                ParseInteger(token, "lineHeight", m_lineHeight);
                ParseInteger(token, "scaleW", m_textureWidth);
                ParseInteger(token, "scaleH", m_textureHeight);
            }
        }
        else if (type == "char")
        {
            int id{};
            FontGlyph glyph;
            std::string token;
            while (tokens >> token)
            {
                ParseInteger(token, "id", id);
                ParseInteger(token, "x", glyph.x);
                ParseInteger(token, "y", glyph.y);
                ParseInteger(token, "width", glyph.width);
                ParseInteger(token, "height", glyph.height);
                ParseInteger(token, "xoffset", glyph.xOffset);
                ParseInteger(token, "yoffset", glyph.yOffset);
                ParseInteger(token, "xadvance", glyph.xAdvance);
            }
            m_glyphs.emplace(static_cast<uint32_t>(id), glyph);
        }
    }

    if (texturePath.empty())
    {
        Logger::Error("FontAssetにpage定義がありません: " + path.string());
        return false;
    }

    m_texture = TextureManager::LoadTexture(texturePath.string());
    if (!m_texture.IsValid() || m_lineHeight <= 0 || m_textureWidth <= 0 || m_textureHeight <= 0)
    {
        Logger::Error("FontAssetの形式またはTextureが不正です: " + path.string());
        return false;
    }
    return true;
}

FontAsset* FontManager::Load(const std::filesystem::path& path)
{
    const std::string key = path.lexically_normal().generic_string();
    if (const auto found = g_fontAssets.find(key); found != g_fontAssets.end())
    {
        return found->second.get();
    }

    std::unique_ptr<FontAsset> asset = std::make_unique<FontAsset>();
    if (!asset->LoadFromFile(path))
    {
        return nullptr;
    }
    FontAsset* const result = asset.get();
    g_fontAssets.emplace(key, std::move(asset));
    return result;
}

void FontManager::Clear()
{
    g_fontAssets.clear();
}
