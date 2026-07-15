#include "Component/PostEffect.h"
#include "System/renderer.h"

ComPtr<ID3D11VertexShader> PostEffect::vertexShader;
ComPtr<ID3D11InputLayout> PostEffect::inputLayout;
ComPtr<ID3D11Buffer> PostEffect::vertexBuffer;

D3D11_PRIMITIVE_TOPOLOGY PostEffect::originalTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
BlendMode PostEffect::originalBlendMode = BlendMode::AlphaBlend;
void PostEffect::Init()
{
    Renderer::CreateVertexShader(
        vertexShader.GetAddressOf(),
        inputLayout.GetAddressOf(),
        "BuildedShader/FullScreen_VS.cso");

    // 頂点バッファの作成
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(FULLSCREEN_VERTICES);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = FULLSCREEN_VERTICES;

    HRESULT hr = Renderer::GetDevice()->CreateBuffer(&desc, &initData, vertexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Failed to create vertex buffer for PostEffect.", "Error", MB_OK | MB_ICONERROR);
    }
}

void PostEffect::PostEffectPass_Begin()
{
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    // ブレンドモードを保存してアルファブレンドに設定
    originalBlendMode = Renderer::GetCurrentBlendMode();
    Renderer::SetBlendMode(BlendMode::None);
    Renderer::SetDepthEnable(false);

    // シェーダーの設定
    context->VSSetShader(vertexShader.Get(), nullptr, 0);
    context->IASetInputLayout(inputLayout.Get());
    // 頂点バッファの設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    // プリミティブトポロジーの設定
    context->IAGetPrimitiveTopology(&originalTopology);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void PostEffect::PostEffectPass_End()
{
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(originalTopology);
    Renderer::SetBlendMode(originalBlendMode);
}

PostEffect::PostEffect(const char* shaderFileName)
{
    Renderer::CreatePixelShader(m_PixelShader.GetAddressOf(), shaderFileName);
}


void PostEffect::Apply()
{
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    // 前回のRTVを解除
    // srcをPrepare内でSRVとして使用可能にする
    context->OMSetRenderTargets(0, nullptr, nullptr);

    // エフェクト固有の中間処理
    Prepare();

    // 共通の最終出力先
    Renderer::SetDstRTV();

    // エフェクト固有のSRV設定など
    SetUp();

    // メインのピクセルシェーダーを設定
    context->PSSetShader(m_PixelShader.Get(), nullptr, 0);

    // 描画済みのテクスチャをSRVとして設定
    ID3D11ShaderResourceView* srcSRV = Renderer::GetSrcSRV();
    context->PSSetShaderResources(0, 1, &srcSRV);

    context->Draw(4, 0);

    // 基底クラスが使ったt0を解除
    ID3D11ShaderResourceView* nullSRV = nullptr;
    context->PSSetShaderResources(0, 1, &nullSRV);

    // 派生クラスが使った追加SRVなどを解除
    Cleanup();

    Renderer::SwapBuffers();
}

Bloom::Bloom() : PostEffect(BLOOM_SHADER_FILE)
{
    ID3D11Device* device = Renderer::GetDevice();
    // 輝度抽出用のテクスチャを作成
    ID3D11Texture2D* bloomTexture = nullptr;
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = SCREEN_WIDTH * 0.5f; // 半分の解像度で作成
    desc.Height = SCREEN_HEIGHT * 0.5f;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&desc, nullptr, &bloomTexture);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Failed to create bloom texture.", "Error", MB_OK | MB_ICONERROR);
    }

    // 輝度抽出用のRTVを作成
    hr = device->CreateRenderTargetView(bloomTexture, nullptr, m_bloomRTV.GetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Failed to create bloom RTV.", "Error", MB_OK | MB_ICONERROR);
    }

    // 輝度のSRVを作成
    hr = device->CreateShaderResourceView(bloomTexture, nullptr, m_bloomSRV.GetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "Failed to create bloom SRV.", "Error", MB_OK | MB_ICONERROR);
    }

    // 作成したテクスチャは不要なので解放
    if (bloomTexture)
    {
        bloomTexture->Release();
    }

    // 輝度抽出&横ブラー用のピクセルシェーダーを作成
    Renderer::CreatePixelShader(m_blurPixelShader.GetAddressOf(), BLOOM_BLUR_SHADER_FILE);
}

void Bloom::Prepare()
{
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();

    // レンダーターゲットを輝度抽出用のRTVに切り替え
    context->OMSetRenderTargets(1, m_bloomRTV.GetAddressOf(), nullptr);

    // ビューポートを半分の解像度に設定
    D3D11_VIEWPORT originalViewport{};
    D3D11_VIEWPORT bloomViewport{};
    bloomViewport.Width = SCREEN_WIDTH * 0.5f;
    bloomViewport.Height = SCREEN_HEIGHT * 0.5f;
    bloomViewport.MinDepth = 0.0f;
    bloomViewport.MaxDepth = 1.0f;

    // 元のビューポートを保存してから、半分の解像度のビューポートに切り替え
    UINT viewportNum = 1;
    context->RSGetViewports(&viewportNum, &originalViewport);
    context->RSSetViewports(1, &bloomViewport);

    // 描画済みのテクスチャをSRVとして設定
    ID3D11ShaderResourceView* srcSRV = Renderer::GetSrcSRV();
    context->PSSetShaderResources(0, 1, &srcSRV);

    // 輝度抽出用&横ブラー用のピクセルシェーダーを設定
    context->PSSetShader(m_blurPixelShader.Get(), nullptr, 0);

    // 輝度抽出&横ブラーの描画
    context->Draw(4, 0);

    // 元のビューポートに戻す
    context->RSSetViewports(1, &originalViewport);

    // Apply で 縦ブラーを適用して加算
}

void Bloom::SetUp()
{
    // 輝度抽出済のテクスチャをSRVとして設定
    Renderer::GetDeviceContext()->PSSetShaderResources(1, 1, m_bloomSRV.GetAddressOf());
}

void Bloom::Cleanup()
{
    // SRVを解除
    ID3D11ShaderResourceView* nullSRV = nullptr;
    Renderer::GetDeviceContext()->PSSetShaderResources(1, 1, &nullSRV);
}
