#pragma once

#include "Graphics/Material.h"
#include "System/Core/ComponentRegistry.h"
#include "System/Rendering/RendererBase.h"

class FontAsset;

enum class UITextHorizontalAlignment
{
    Left,
    Center,
    Right,
};

enum class UITextVerticalAlignment
{
    Top,
    Center,
    Bottom,
};

// BMFontのGlyphをCanvas配下へ描画するテキストComponent。
class UIText final : public RendererBase
{
    REGISTER_COMPONENT(UIText, "UI Text");

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_colorBuffer;
    Material* m_material{};
    std::string m_materialPath{ "Assets/Materials/Text.material" };
    std::string m_loadedMaterialPath;
    FontAsset* m_font{};
    std::string m_fontPath;
    std::string m_loadedFontPath;
    std::string m_text;
    float m_fontSize{ 32.0f };
    Color m_color{ Color::White() };
    Color m_outlineColor{ Color::Black() };
    float m_outlineWidth{};
    int m_horizontalAlignment{ static_cast<int>(UITextHorizontalAlignment::Left) };
    int m_verticalAlignment{ static_cast<int>(UITextVerticalAlignment::Top) };

    void CreateVertexBuffers();

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    UIText() = delete;
    explicit UIText(GameObject* owner);

    void OnLoaded() override;
    void OnEditorUpdate() override;
    void Update() override;
    void Draw() override;
    Material* GetMaterial(uint32_t slot = 0) override;
    void SetMaterial(uint32_t slot, Material* material) override;

    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() const { return m_text; }
    void SetColor(const Color& color) { m_color = color; }
    const Color& GetColor() const { return m_color; }
    void SetOutlineColor(const Color& color) { m_outlineColor = color; }
    const Color& GetOutlineColor() const { return m_outlineColor; }
    void SetOutlineWidth(float width) { m_outlineWidth = Math::Max(0.0f, width); }
    float GetOutlineWidth() const { return m_outlineWidth; }

    // 文字のレイアウト原点を維持したサイズ演出用。UITransformのScaleとは独立する。
    void SetFontSize(float fontSize) { m_fontSize = fontSize > 0.0f ? fontSize : 0.0f; }
    float GetFontSize() const { return m_fontSize; }

    void SetHorizontalAlignment(UITextHorizontalAlignment alignment);
    UITextHorizontalAlignment GetHorizontalAlignment() const;
    int GetHorizontalAlignmentValue() const { return m_horizontalAlignment; }
    void SetHorizontalAlignmentValue(int alignment);

    void SetVerticalAlignment(UITextVerticalAlignment alignment);
    UITextVerticalAlignment GetVerticalAlignment() const;
    int GetVerticalAlignmentValue() const { return m_verticalAlignment; }
    void SetVerticalAlignmentValue(int alignment);
};
