#pragma once

#include "Graphics/ModelAsset.h"
#include "Graphics/Material.h"
#include "System/Core/ComponentRegistry.h"

#include <string>
#include <string_view>

class GraphicsShaderPass;

// PostProcessはMaterialの実行順・出力先・画面入力だけを制御する。
class PostProcess : public Component
{
    static Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    static Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;

    Material* m_material{};
    std::string m_name;
    std::string m_materialPath;

    bool BindFullscreenVertices(const GraphicsShaderPass& shaderPass) const;
    void ClearBoundTextures(const GraphicsShaderPass& shaderPass) const;
    void BindSourceTexture() const;
    void DrawFullscreen() const;
    virtual void OnMaterialChanged() {}

protected:
    explicit PostProcess(GameObject* owner) : Component(owner) {}

    void RegisterFields(FieldRegistry& fields) override;

    // 画面バッファへ描画し、成功時にRendererのPing-Pongを入れ替える。
    bool ExecuteScreenPassAndSwap(size_t passIndex);
    bool ExecuteScreenPassAndSwap(std::string_view passTag);

    // 中間RenderTextureへ描画する。現在の画面入力は入れ替えない。
    bool ExecuteMiddlePass(size_t passIndex, TextureHandle renderTarget);
    bool ExecuteMiddlePass(std::string_view passTag, TextureHandle renderTarget);

public:
    static void Init();
    static void Uninit();

    void Update() final override {}
    void LateUpdate() final override {}
    void Draw() final override {}

    void OnLoaded() final override;
    void OnEnable() final override;
    void OnDisable() final override;
    void OnDestroy() final override;

    std::string_view GetName() const { return m_name; }
    Material* GetMaterial() const { return m_material; }
    const std::string& GetMaterialPath() const { return m_materialPath; }
    void SetMaterial(Material* material);
    void SetMaterialPath(const std::string& path);
    virtual void Resize(UINT width, UINT height) { (void)width; (void)height; }
    virtual void Apply() = 0;
};

