#include "pch.h"

#include "System/GM_Time.h"
#include "Component/TrailRenderer.h"
#include "Component/Transform.h"
#include "Component/Camera.h"


void TrailRenderer::UpdateSegments()
{
    if(m_segments.size() > 0)
    {
        // 各セグメントの経過時間を更新
        for (auto& segment : m_segments)
        {
            segment.elapsedTime += Time::GetDeltaTime();
        }
        // 寿命を超えたセグメントを削除
        while (!m_segments.empty() && m_segments.front().elapsedTime > m_trailLifetime)
        {
            m_segments.pop_front();
        }
    }

    Vector3 currentPosition = transform()->GetPosition() + m_offset;
    // 新しいセグメントを追加する条件
    if (m_segments.empty() ||
        (currentPosition - m_segments.back().position).LengthSqr() >= m_minSegmentDistance * m_minSegmentDistance)
    {
        if( m_segments.size() >= MAX_SEGMENTS)
            m_segments.pop_front(); // 最大セグメント数を超えた場合、古いセグメントを削除

        m_segments.emplace_back(currentPosition);
    }
}

TrailRenderer::TrailRenderer(
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
            return;
        }
        CreateShaderResourceView(
            Renderer::GetDevice(),
            image.GetImages(),
            image.GetImageCount(),
            metadata,
            m_texture.GetAddressOf());
    }

    // 頂点バッファの設定
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(VERTEX_3D) * MAX_SEGMENTS * 2; // 各セグメントは2つの頂点を持つ
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = Renderer::GetDevice()->CreateBuffer(&bd, nullptr, m_vertexBuffer.GetAddressOf());

    if (FAILED(hr))
    {
        MessageBoxA(nullptr, "頂点バッファの作成に失敗しました。", "エラー", MB_OK | MB_ICONERROR);
    }

    m_renderPass = RenderPass::Transparent; // トレイルは透明パスで描画
    m_blendMode = BlendMode::AlphaBlend;      // アルファブレンドを使用
    m_cullMode = CullMode::None;          // カリングなし
}

void TrailRenderer::Draw()
{
    UpdateSegments();

    if (m_segments.size() < 2)
        return; // セグメントが2つ未満の場合は描画しない

    const int segmentCount = static_cast<int>(m_segments.size());
    const int vertexCount = segmentCount * 2; // 各セグメントは2つの頂点を持つ
    ID3D11DeviceContext* context = Renderer::GetDeviceContext();
    // 頂点データの作成
    std::vector<VERTEX_3D> vertices(vertexCount);

    // カメラ方向
    Vector3 cameraPosition = Camera::GetMainCamera()->transform()->GetPosition();

    for (int i = 0; i < segmentCount; ++i)
    {
        // セグメントの位置と方向を計算
        const Vector3& pos = m_segments[i].position;
        Vector3 dir;
        if (i == 0)
        {
            dir = m_segments[i + 1].position - m_segments[i].position;
        }
        else if (i == segmentCount - 1)
        {
            dir = m_segments[i].position - m_segments[i - 1].position;
        }
        else
        {
            dir = m_segments[i + 1].position - m_segments[i - 1].position;
        }
        dir.Normalize();

        Vector3 toCamera = cameraPosition - pos;
        toCamera.Normalize();

        // トレイルの左右方向を求める
        Vector3 normal;

        switch (m_faceMode)
        {
        case TrailRenderer::CameraFacing:
            normal = toCamera;
            break;

        case TrailRenderer::FixedDirection:
            normal = m_fixedDirection;
            normal.Normalize();
            break;
        }

        Vector3 side = Vector3::Cross(normal, dir);
        side.Normalize();
        // セグメントの経過時間に基づいて幅と色を計算
        float ageRate = m_segments[i].elapsedTime / m_trailLifetime;

        // Curve 
        float curvedWidth = m_widthCurve.Evaluate(ageRate);
        Color color = m_colorCurve.Evaluate(ageRate);

        Vector3 leftPos = pos + side * curvedWidth * m_width * 0.5f;
        Vector3 rightPos = pos - side * curvedWidth * m_width * 0.5f;
        // テクスチャ座標の計算
        float v = static_cast<float>(i) / static_cast<float>(segmentCount - 1);

        vertices[i * 2 + 0].Position = XMFLOAT3(leftPos.x, leftPos.y, leftPos.z);
        vertices[i * 2 + 0].Normal = XMFLOAT3(normal.x, normal.y, normal.z);
        vertices[i * 2 + 0].Diffuse = XMFLOAT4(color.r, color.g, color.b, color.a);
        vertices[i * 2 + 0].TexCoord = XMFLOAT2(0.0f, v);

        vertices[i * 2 + 1].Position = XMFLOAT3(rightPos.x, rightPos.y, rightPos.z);
        vertices[i * 2 + 1].Normal = XMFLOAT3(normal.x, normal.y, normal.z);
        vertices[i * 2 + 1].Diffuse = XMFLOAT4(color.r, color.g, color.b, color.a);
        vertices[i * 2 + 1].TexCoord = XMFLOAT2(1.0f, v);
    }

    D3D11_MAPPED_SUBRESOURCE msr{};
    context->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
    memcpy(msr.pData, vertices.data(), sizeof(VERTEX_3D) * vertexCount);
    context->Unmap(m_vertexBuffer.Get(), 0);

    UINT stride = sizeof(VERTEX_3D);
    UINT offset = 0;

    Renderer::SetWorldMatrix(XMMatrixIdentity());

    Renderer::SetMaterial(
        {   XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // Ambient
            XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // Diffuse
            XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // Specular
            XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), // Emission
            32.0f,                           // Shininess
            TRUE                             // TextureEnable
        });

    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    ID3D11ShaderResourceView* texture = m_texture.Get();
    context->PSSetShaderResources(0, 1, &texture);

    context->Draw(vertexCount, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}
