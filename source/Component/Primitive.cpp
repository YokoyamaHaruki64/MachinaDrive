#include "Component/Primitive.h"

#include "System/renderer.h"
#include "Component/Transform.h"

const char* Primitive::DEFAULT_VERTEX_SHADER_FILE = "BuildedShader/unlitTextureVS.cso";
const char* Primitive::DEFAULT_PIXEL_SHADER_FILE = "BuildedShader/unlitTexturePS.cso";

void Primitive::LoadTexture(const wchar_t* textureFile)
{
    if(textureFile == nullptr)
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

        return;
    }
    // テクスチャの読み込み
    TexMetadata metadata{};
    ScratchImage image{};
    HRESULT hr = LoadFromWICFile(textureFile, WIC_FLAGS_NONE, &metadata, image);
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "テクスチャの読み込みに失敗しました。", "エラー", MB_OK | MB_ICONERROR);
        return;
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

void Primitive::CreateVertexShaderAndInputLayout(const char* vertexShaderFile)
{
   Renderer::CreateVertexShader(
        m_vertexShader.GetAddressOf(),
        m_inputLayout.GetAddressOf(),
        vertexShaderFile);
}

void Primitive::CreatePixelShader(const char* pixelShaderFile)
{
    Renderer::CreatePixelShader(
        m_pixelShader.GetAddressOf(),
        pixelShaderFile);
}

void Primitive::CreateVertexBuffer(const void* vertexData, size_t vertexDataSize)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = static_cast<UINT>(vertexDataSize);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertexData;

    HRESULT hr = Renderer::GetDevice()->CreateBuffer(&bd, &initData, m_vertexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "頂点バッファの作成に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
        return;
    }
}

void Primitive::CreateIndexBuffer(const unsigned int* indexData, size_t indexDataSize)
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = static_cast<UINT>(indexDataSize);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = indexData;
    HRESULT hr = Renderer::GetDevice()->CreateBuffer(&bd, &initData, m_indexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "インデックスバッファの作成に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
        return;
    }
}

void Primitive::Draw()
{
    // 入力レイアウトとシェーダーの設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_inputLayout.Get());
    Renderer::GetDeviceContext()->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // ワールド行列の設定
    Transform* t = transform();
    if (t)
    {
        XMMATRIX worldMatrix =
            XMMatrixScaling(
                t->GetScale().x,
                t->GetScale().y,
                t->GetScale().z)
            *
            XMMatrixRotationRollPitchYaw(
                t->GetRotation().x,
                t->GetRotation().y,
                t->GetRotation().z)
            *
            XMMatrixTranslation(
                t->GetPosition().x,
                t->GetPosition().y,
                t->GetPosition().z);

        Renderer::SetWorldMatrix(worldMatrix);
    }
    // マテリアルの設定
    MATERIAL material{};
    material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    material.TextureEnable = true;
    Renderer::SetMaterial(material);

    // 頂点バッファの設定
    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;
    Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    // インデックスバッファの設定
    Renderer::GetDeviceContext()->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    // シェーダーリソースビューの設定
    Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, m_texture.GetAddressOf());
    // 描画コール
    Renderer::GetDeviceContext()->DrawIndexed(m_indexCount, 0, 0);
}

Quad::Quad(GameObject* owner,
    const wchar_t* textureFile,
    const char* vertexShaderFile,
    const char* pixelShaderFile) : Primitive(owner)
{
    LoadTexture(textureFile);

    VERTEX_3D vertex[4] = {};

    vertex[0].Position = XMFLOAT3(-0.5f,0.5f,0.0f);
    vertex[0].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

    vertex[1].Position = XMFLOAT3(0.5f, 0.5f, 0.0f);
    vertex[1].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[1].TexCoord = XMFLOAT2(1.0f, 0.0f);

    vertex[2].Position = XMFLOAT3(-0.5f, -0.5f, 0.0f);
    vertex[2].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[2].TexCoord = XMFLOAT2(0.0f, 1.0f);

    vertex[3].Position = XMFLOAT3(0.5f, -0.5f, 0.0f);
    vertex[3].Normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
    vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[3].TexCoord = XMFLOAT2(1.0f, 1.0f);

    // 頂点バッファの設定
    CreateVertexBuffer(vertex, sizeof(vertex));
    CreateIndexBuffer(QUAD_INDEX_BUFFER, sizeof(QUAD_INDEX_BUFFER));

    // 頂点シェーダーの作成と入力レイアウトの設定
    CreateVertexShaderAndInputLayout(vertexShaderFile);

    // ピクセルシェーダーの作成
    CreatePixelShader(pixelShaderFile);

    m_indexCount = QUAD_INDEX_COUNT;
}

Plane::Plane(
    GameObject* owner,
    const wchar_t* textureFile,
    const char* vertexShaderFile,
    const char* pixelShaderFile) : Primitive(owner)
{
    LoadTexture(textureFile);

    VERTEX_3D vertex[4];

    const float TEX_REPEAT = 30.0f; // テクスチャの繰り返し回数

    // 頂点データの設定 (左上)
    vertex[0].Position = XMFLOAT3(-50.0f, 0.0f, 50.0f);
    vertex[0].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    vertex[0].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[0].TexCoord = XMFLOAT2(0.0f, 0.0f);

    // 頂点データの設定 (右上)
    vertex[1].Position = XMFLOAT3(50.0f, 0.0f, 50.0f);
    vertex[1].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    vertex[1].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[1].TexCoord = XMFLOAT2(TEX_REPEAT, 0.0f);

    // 頂点データの設定 (左下)
    vertex[2].Position = XMFLOAT3(-50.0f, 0.0f, -50.0f);
    vertex[2].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    vertex[2].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[2].TexCoord = XMFLOAT2(0.0f, TEX_REPEAT);

    // 頂点データの設定 (右下)
    vertex[3].Position = XMFLOAT3(50.0f, 0.0f, -50.0f);
    vertex[3].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
    vertex[3].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    vertex[3].TexCoord = XMFLOAT2(TEX_REPEAT, TEX_REPEAT);

    // 頂点バッファの設定
    CreateVertexBuffer(vertex, sizeof(vertex));
    CreateIndexBuffer(PLANE_INDEX_BUFFER, sizeof(PLANE_INDEX_BUFFER));

    // 頂点シェーダーの作成と入力レイアウトの設定
    CreateVertexShaderAndInputLayout(vertexShaderFile);

    // ピクセルシェーダーの作成
    CreatePixelShader(pixelShaderFile);

    m_indexCount = PLANE_INDEX_COUNT;
}

Cube::Cube(
    GameObject* owner,
    const wchar_t* textureFile,
    const char* vertexShaderFile,
    const char* pixelShaderFile) : Primitive(owner)
{
    LoadTexture(textureFile);

    VERTEX_3D vertex[24] = {};

    auto SetNormalSurface = [&](int startIndex, XMFLOAT3 normal)
        {
            for (int i = 0; i < 4; i++)
            {
                vertex[startIndex + i].Normal = normal;
            }
        };

    // テクスチャ座標を設定するラムダ関数
    // vertPosは各面の中でのindex(0~3)
    auto SetTexCoord = [&](int vertPos, XMFLOAT2 texCoord)
        {
            for (int i = 0; i < 24; i += 4)
            {
                vertex[vertPos + i].TexCoord = texCoord;
            }
        };

    // 天
    vertex[0].Position = XMFLOAT3(-0.5f, 0.5f, 0.5f);
    vertex[1].Position = XMFLOAT3(0.5f, 0.5f, 0.5f);
    vertex[2].Position = XMFLOAT3(-0.5f, 0.5f, -0.5f);
    vertex[3].Position = XMFLOAT3(0.5f, 0.5f, -0.5f);

    SetNormalSurface(0, XMFLOAT3(0.0f, 1.0f, 0.0f));

    // 地
    vertex[4].Position = XMFLOAT3(-0.5f, -0.5f, -0.5f);
    vertex[5].Position = XMFLOAT3(0.5f, -0.5f, -0.5f);
    vertex[6].Position = XMFLOAT3(-0.5f, -0.5f, 0.5f);
    vertex[7].Position = XMFLOAT3(0.5f, -0.5f, 0.5f);
    SetNormalSurface(4, XMFLOAT3(0.0f, -1.0f, 0.0f));

    // 奥
    vertex[8].Position = XMFLOAT3(0.5f, 0.5f, 0.5f);
    vertex[9].Position = XMFLOAT3(-0.5f, 0.5f, 0.5f);
    vertex[10].Position = XMFLOAT3(0.5f, -0.5f, 0.5f);
    vertex[11].Position = XMFLOAT3(-0.5f, -0.5f, 0.5f);
    SetNormalSurface(8, XMFLOAT3(0.0f, 0.0f, 1.0f));

    // 手前
    vertex[12].Position = XMFLOAT3(-0.5f, 0.5f, -0.5f);
    vertex[13].Position = XMFLOAT3(0.5f, 0.5f, -0.5f);
    vertex[14].Position = XMFLOAT3(-0.5f, -0.5f, -0.5f);
    vertex[15].Position = XMFLOAT3(0.5f, -0.5f, -0.5f);
    SetNormalSurface(12, XMFLOAT3(0.0f, 0.0f, -1.0f));

    // 右
    vertex[16].Position = XMFLOAT3(0.5f, 0.5f, -0.5f);
    vertex[17].Position = XMFLOAT3(0.5f, 0.5f, 0.5f);
    vertex[18].Position = XMFLOAT3(0.5f, -0.5f, -0.5f);
    vertex[19].Position = XMFLOAT3(0.5f, -0.5f, 0.5f);
    SetNormalSurface(16, XMFLOAT3(1.0f, 0.0f, 0.0f));

    // 左
    vertex[20].Position = XMFLOAT3(-0.5f, 0.5f, 0.5f);
    vertex[21].Position = XMFLOAT3(-0.5f, 0.5f, -0.5f);
    vertex[22].Position = XMFLOAT3(-0.5f, -0.5f, 0.5f);
    vertex[23].Position = XMFLOAT3(-0.5f, -0.5f, -0.5f);
    SetNormalSurface(20, XMFLOAT3(-1.0f, 0.0f, 0.0f));

    // テクスチャ座標の設定
    SetTexCoord(0, XMFLOAT2(0.0f, 0.0f));
    SetTexCoord(1, XMFLOAT2(1.0f, 0.0f));
    SetTexCoord(2, XMFLOAT2(0.0f, 1.0f));
    SetTexCoord(3, XMFLOAT2(1.0f, 1.0f));

    // 色の設定
    for (int i = 0; i < 24; i++)
    {
        vertex[i].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    // 頂点バッファの設定
    CreateVertexBuffer(vertex, sizeof(vertex));
    CreateIndexBuffer(CUBE_INDEX_BUFFER, sizeof(CUBE_INDEX_BUFFER));

    // 頂点シェーダーの作成と入力レイアウトの設定
    CreateVertexShaderAndInputLayout(vertexShaderFile);

    // ピクセルシェーダーの作成
    CreatePixelShader(pixelShaderFile);

    m_indexCount = CUBE_INDEX_COUNT;
}
