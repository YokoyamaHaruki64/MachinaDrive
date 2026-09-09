#pragma once

#include "Graphics/Material.h"
#include "System/Core/ComponentRegistry.h"
#include "System/Rendering/RendererBase.h"

class FontAsset;

// BMFontのGlyphをCanvas配下へ描画するテキストComponent。
class UIText final : public RendererBase
{
    REGISTER_COMPONENT(UIText, "UI Text");

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_colorBuffer;
    Material* m_material{};
    std::string m_materialPath{ "Assets/Materials/UI.material" };
    std::string m_loadedMaterialPath;
    FontAsset* m_font{};
    std::string m_fontPath;
    std::string m_loadedFontPath;
    std::string m_text;
    float m_fontSize{ 32.0f };
    Color m_color{ Color::White() };

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
};
