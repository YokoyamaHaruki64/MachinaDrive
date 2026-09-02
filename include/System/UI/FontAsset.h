#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include "Graphics/GraphicInc.h"

struct FontGlyph
{
    int x{};
    int y{};
    int width{};
    int height{};
    int xOffset{};
    int yOffset{};
    int xAdvance{};
};

// .fntと同名pngを対にして保持する、BMFont用の読み込み済みアセット。
class FontAsset final
{
    std::filesystem::path m_path;
    TextureHandle m_texture;
    int m_lineHeight{};
    int m_textureWidth{};
    int m_textureHeight{};
    std::unordered_map<uint32_t, FontGlyph> m_glyphs;

    bool LoadFromFile(const std::filesystem::path& path);

    friend class FontManager;

public:
    const std::filesystem::path& GetPath() const { return m_path; }
    TextureHandle GetTexture() const { return m_texture; }
    int GetLineHeight() const { return m_lineHeight; }
    int GetTextureWidth() const { return m_textureWidth; }
    int GetTextureHeight() const { return m_textureHeight; }
    const FontGlyph* FindGlyph(uint32_t codePoint) const;
};

class FontManager final
{
public:
    static FontAsset* Load(const std::filesystem::path& path);
    static void Clear();
};
