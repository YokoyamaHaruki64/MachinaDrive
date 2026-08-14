#pragma once

#include "Graphics/ModelAsset.h"
#include "Graphics/Material.h"

#include <string_view>

class GraphicsShaderPass;

// PostProcessはMaterialの実行順・出力先・画面入力だけを制御する。
class PostProcess
{
    static Microsoft::WRL::ComPtr<ID3D11Buffer> m_positionBuffer;
    static Microsoft::WRL::ComPtr<ID3D11Buffer> m_texCoordBuffer;

    Material* m_material{};

    bool BindFullscreenVertices(const GraphicsShaderPass& shaderPass) const;
    void ClearBoundTextures(const GraphicsShaderPass& shaderPass) const;
    void BindSourceTexture() const;
    void DrawFullscreen() const;

protected:
    explicit PostProcess(const char* shaderFilePath);

    // 画面バッファへ描画し、成功時にRendererのPing-Pongを進める。
    bool ExecutePass(size_t passIndex);
    bool ExecutePass(std::string_view passTag);

    // 中間RenderTextureへ描画する。中間出力は現在の画面入力を変更しない。
    bool ExecutePass(size_t passIndex, TextureHandle renderTarget);
    bool ExecutePass(std::string_view passTag, TextureHandle renderTarget);

    Material* GetMaterial() { return m_material; }
    const Material* GetMaterial() const { return m_material; }

public:
    static void Init();
    static void Uninit();

    virtual ~PostProcess() = default;
    virtual void Apply() = 0;
};

class Bloom : public PostProcess
{
    static constexpr const char* SHADER_FILE_PATH = "shader/PostProcessBloom.shader";
    TextureHandle m_bloomTexture{};

    size_t m_bloomExtractPassIndex{};
    size_t m_compositePassIndex{};

public:
    Bloom();
    void Apply() override;
};
