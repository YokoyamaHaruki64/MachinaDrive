#include "pch.h"

#include "Component/SpriteRenderer.h"
#include "System/renderer.h"

const char* SpriteRenderer::DEFAULT_VERTEX_SHADER_FILE = "BuildedShader/unlitTextureVS.cso";
const char* SpriteRenderer::DEFAULT_PIXEL_SHADER_FILE = "BuildedShader/unlitTexturePS.cso";

SpriteRenderer::SpriteRenderer(
    GameObject* owner,
    const wchar_t* textureFile,
    const char* vertexShaderFile,
    const char* pixelShaderFile)
    : RendererBase(owner)
{
    Renderer::CreateVertexShader(
        m_vertexShader.GetAddressOf(),
        m_inputLayout.GetAddressOf(),
        vertexShaderFile);

    Renderer::CreatePixelShader(
        m_pixelShader.GetAddressOf(),
        pixelShaderFile);

    if (textureFile == nullptr)
    {
        // 白いフォールバックテクスチャの初期化
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = 1;
        desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        uint32_t whitePixel = 0xFFFFFFFF; // RGBAで白色
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &whitePixel;
        ComPtr<ID3D11Texture2D> texture;
        HRESULT hr = Renderer::GetDevice()->CreateTexture2D(&desc, &initData, texture.GetAddressOf());
        if (FAILED(hr))
        {
            MessageBoxA(nullptr, "フォールバックテクスチャの作成に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
        }
        // シェーダーリソースビューの作成
        hr = Renderer::GetDevice()->CreateShaderResourceView(
            texture.Get(),
            nullptr,
            m_texture.GetAddressOf());
        if (FAILED(hr))
        {
            MessageBoxA(nullptr, "フォールバックテクスチャのシェーダーリソースビューの作成に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
        }

    }
    else
    {

        // テクスチャの読み込み
        TexMetadata metadata{};
        ScratchImage image{};
        HRESULT hr = LoadFromWICFile(textureFile, WIC_FLAGS_NONE, &metadata, image);
        if (FAILED(hr))
        {
            MessageBoxW(nullptr,
                (L"テクスチャの読み込みに失敗しました: " + std::wstring(textureFile)).c_str(),
                L"エラー",
                MB_OK | MB_ICONERROR);
        }
        CreateShaderResourceView(
            Renderer::GetDevice(),
            image.GetImages(),
            image.GetImageCount(),
            metadata,
            m_texture.GetAddressOf());

        m_textureWidth = static_cast<int>(metadata.width);
        m_textureHeight = static_cast<int>(metadata.height);
    }

    // 頂点バッファの作成
    VERTEX_3D vertices[4];

    vertices[0].Position = XMFLOAT3(-0.5f, -0.5f, 0.0f);
    vertices[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

    vertices[1].Position = XMFLOAT3(0.5f, -0.5f, 0.0f);
    vertices[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

    vertices[2].Position = XMFLOAT3(-0.5f, 0.5f, 0.0f);
    vertices[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

    vertices[3].Position = XMFLOAT3(0.5f, 0.5f, 0.0f);
    vertices[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

    for(int i = 0; i < 4; ++i)
    {
        vertices[i].Normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
        vertices[i].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(vertices);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    HRESULT hr = Renderer::GetDevice()->CreateBuffer(&desc, &initData, m_vertexBuffer.GetAddressOf());

    if( FAILED(hr) )
    {
        MessageBoxA(nullptr, "頂点バッファの作成に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
    }
        
}

void SpriteRenderer::Draw()
{
    // 入力レイアウトとシェーダーの設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_inputLayout.Get());
    Renderer::GetDeviceContext()->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // 2Dのビュー行列と射影行列を設定
    // 3Dのビュー行列と射影行列を保存
    XMMATRIX originalViewMatrix = Renderer::GetViewMatrix();
    XMMATRIX originalProjectionMatrix = Renderer::GetProjectionMatrix();
    Renderer::SetWorldViewProjection2D();

    // ワールド行列の設定
    Renderer::SetWorldMatrix(CalcWorldMatrix());

    // マテリアルの設定
    Renderer::SetMaterial(m_material);

    // UVの設定
    Renderer::SetUV(XMFLOAT4(m_uvWidth, m_uvHeight, m_uvOffsetX, m_uvOffsetY));

    // 頂点バッファの設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    // シェーダーリソースビューの設定
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, m_texture.GetAddressOf());

    // プリミティブトポロジの設定
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // 描画
    Renderer::GetDeviceContext()->Draw(4, 0);

    // ビュー行列と射影行列を元に戻す
    Renderer::SetViewMatrix(originalViewMatrix);
    Renderer::SetProjectionMatrix(originalProjectionMatrix);

    // プリミティブトポロジを元に戻す
    Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
