#pragma once

#include "Graphics/Material.h"
#include "System/Core/ComponentRegistry.h"
#include "System/Rendering/RendererBase.h"

// Canvas配下でTextureHandleを直接t2へ束縛する画像描画・Hit判定Component。
class UIImage final : public RendererBase
{
    REGISTER_COMPONENT(UIImage, "UI Image");

    Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_colorBuffer;

    Material* m_material{};
    std::string m_materialPath{ "Assets/Materials/UI.material" };
    std::string m_loadedMaterialPath;
    TextureHandle m_texture;
    std::string m_texturePath;
    std::string m_loadedTexturePath;
    Color m_color{ Color::White() };
    bool m_raycastTarget = true;
    bool m_dragEnabled = false;

    bool m_pointerPressed{};
    bool m_pointerHeld{};
    bool m_pointerReleased{};
    bool m_dragStarted{};
    bool m_dragging{};
    bool m_dragEnded{};
    bool m_pointerCaptured{};
    Vector2 m_pressPosition{};

    bool ContainsPointer() const;
    void CreateVertexBuffers();
    void UpdateSizeFromTexture();

protected:
    void RegisterFields(FieldRegistry& fields) override;

public:
    UIImage() = delete;
    explicit UIImage(GameObject* owner);

    void OnLoaded() override;
    void OnEditorUpdate() override;
    void Update() override;
    void Draw() override;

    Material* GetMaterial(uint32_t slot = 0) override;
    void SetMaterial(uint32_t slot, Material* material) override;

    void SetTexture(TextureHandle texture) { m_texture = texture; }
    TextureHandle GetTexture() const { return m_texture; }
    bool SetTexturePath(std::string path);
    const std::string& GetTexturePath() const { return m_texturePath; }
    void SetColor(const Color& color) { m_color = color; }
    const Color& GetColor() const { return m_color; }
    void SetRaycastTarget(bool value) { m_raycastTarget = value; }
    bool IsRaycastTarget() const { return m_raycastTarget; }
    void SetDragEnabled(bool value) { m_dragEnabled = value; }
    bool IsDragEnabled() const { return m_dragEnabled; }

    bool WasPointerPressed() const { return m_pointerPressed; }
    bool IsPointerPressed() const { return m_pointerHeld; }
    bool WasPointerReleased() const { return m_pointerReleased; }
    bool WasDragStarted() const { return m_dragStarted; }
    bool IsDragging() const { return m_dragging; }
    bool WasDragEnded() const { return m_dragEnded; }
    bool IsPointerOver() const { return ContainsPointer(); }
};
