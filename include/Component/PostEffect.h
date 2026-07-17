#pragma once
#include "System/main.h"
#include "System/renderer.h"

class PostEffect
{
    static constexpr VERTEX_3D FULLSCREEN_VERTICES[4] =
    {
        {
            XMFLOAT3(-1.0f,  1.0f, 0.0f),
            XMFLOAT3(0, 0, -1),
            XMFLOAT4(1, 1, 1, 1),
            XMFLOAT2(0, 0)
        },
        {
            XMFLOAT3(1.0f,  1.0f, 0.0f),
            XMFLOAT3(0, 0, -1),
            XMFLOAT4(1, 1, 1, 1),
            XMFLOAT2(1, 0)
        },
        {
            XMFLOAT3(-1.0f, -1.0f, 0.0f),
            XMFLOAT3(0, 0, -1),
            XMFLOAT4(1, 1, 1, 1),
            XMFLOAT2(0, 1)
        },
        {
            XMFLOAT3(1.0f, -1.0f, 0.0f),
            XMFLOAT3(0, 0, -1),
            XMFLOAT4(1, 1, 1, 1),
            XMFLOAT2(1, 1)
        }
    };
    // ポストエフェクト用のVSと頂点バッファは共有
    static ComPtr<ID3D11VertexShader> vertexShader;
    static ComPtr<ID3D11InputLayout> inputLayout;
    static ComPtr<ID3D11Buffer> vertexBuffer;
    static D3D11_PRIMITIVE_TOPOLOGY originalTopology;
    static BlendMode originalBlendMode;

    ComPtr<ID3D11PixelShader> m_PixelShader;

public:

    static void Init();
    static void PostEffectPass_Begin();
    static void PostEffectPass_End();

    PostEffect() = delete;
    PostEffect(const char* shaderFileName);
    virtual ~PostEffect() = default;
    virtual void Prepare() {};
    virtual void SetUp() {};
    virtual void Cleanup() {};
    void Apply();
};

class Bloom : public PostEffect
{
    static constexpr const char* BLOOM_SHADER_FILE = "BuildedShader/Bloom_PS.cso";
    static constexpr const char* BLOOM_BLUR_SHADER_FILE = "BuildedShader/BloomBlur_PS.cso";
    ComPtr<ID3D11RenderTargetView> m_bloomRTV;
    ComPtr<ID3D11ShaderResourceView> m_bloomSRV;
    ComPtr<ID3D11PixelShader> m_blurPixelShader;
    
public:
    Bloom();
    virtual void Prepare() override;
    virtual void SetUp() override;
    virtual void Cleanup() override;

};
