#include "pch.h"

#include "System/main.h"
#include "System/renderer.h"
#include "System/RendererBase.h"
#include "Graphics/Material.h"
#include "Graphics/TextureManager.h"
#include <io.h>


D3D_FEATURE_LEVEL       Renderer::m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*           Renderer::m_Device{};
ID3D11DeviceContext*    Renderer::m_DeviceContext{};
IDXGISwapChain*         Renderer::m_SwapChain{};
ID3D11Texture2D*        Renderer::m_BackBufferTexture{};
ID3D11DepthStencilView* Renderer::m_DepthStencilView{};

ID3D11Texture2D*            Renderer::m_bufferTexture[2]{};
ID3D11RenderTargetView*     Renderer::m_bufferRTV[2]{};
ID3D11ShaderResourceView*   Renderer::m_bufferSRV[2]{};
int Renderer::srcBufferIndex = 0;
int Renderer::dstBufferIndex = 1;

ID3D11Buffer*			Renderer::m_PerFrameBuffer{};
ID3D11Buffer*			Renderer::m_PerObjectBuffer{};
ID3D11Buffer*			Renderer::m_MaterialBuffer{};
ID3D11Buffer*			Renderer::m_LightingBuffer{};
ID3D11Buffer*           Renderer::m_UVBuffer{};
std::array<ID3D11Buffer*, static_cast<size_t>(VertexStreamType::Count)> Renderer::m_FallbackVertexBuffers{};


std::array<ID3D11DepthStencilState*,
    static_cast<size_t>(DepthTestMode::Count) * static_cast<size_t>(DepthWriteMode::Count)>
    Renderer::m_DepthStates{};

ID3D11RasterizerState* Renderer::m_RasterizerState{};
ID3D11RasterizerState* Renderer::m_RasterizerStateNoCull{};
ID3D11RasterizerState* Renderer::m_RasterizerStateFront{};


ID3D11BlendState*		Renderer::m_BlendState{};
ID3D11BlendState*       Renderer::m_BlendStateAdd{};
ID3D11BlendState*		Renderer::m_BlendStateATC{};
ID3D11SamplerState*		Renderer::m_LinearSampler{};
ID3D11SamplerState*		Renderer::m_ShadowSampler{};

XMMATRIX Renderer::m_ViewMatrix{};
XMMATRIX Renderer::m_ProjectionMatrix{};
PerFrame Renderer::m_PerFrame{};
PerObject Renderer::m_PerObject{};

std::array<std::vector<DrawItem>, static_cast<size_t>(RenderPhase::Count)> Renderer::m_DrawQueues{};
uint64_t Renderer::m_SubmitOrder{};
BlendMode Renderer::m_CurrentBlendMode{ BlendMode::AlphaBlend };
CullMode Renderer::m_CurrentCullMode{ CullMode::Back };

namespace
{
    // 再生成を行わないため、一般的なモデルの頂点数を十分に上回る2^20頂点を確保する。
    constexpr UINT FALLBACK_VERTEX_COUNT = 1u << 20;

    template <typename VertexType>
    ID3D11Buffer* CreateFallbackVertexBuffer(ID3D11Device* device, const VertexType& defaultVertex)
    {
        std::vector<VertexType> vertices(FALLBACK_VERTEX_COUNT, defaultVertex);

        D3D11_BUFFER_DESC description{};
        description.ByteWidth = static_cast<UINT>(sizeof(VertexType) * vertices.size());
        description.Usage = D3D11_USAGE_IMMUTABLE;
        description.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initialData{};
        initialData.pSysMem = vertices.data();

        ID3D11Buffer* buffer{};
        return SUCCEEDED(device->CreateBuffer(&description, &initialData, &buffer)) ? buffer : nullptr;
    }

    constexpr size_t GetDepthStateIndex(DepthTestMode testMode, DepthWriteMode writeMode)
    {
        return static_cast<size_t>(testMode) * 2 + static_cast<size_t>(writeMode);
    }

    D3D11_COMPARISON_FUNC ToD3DComparisonFunc(DepthTestMode mode)
    {
        switch (mode)
        {
        case DepthTestMode::Less:
            return D3D11_COMPARISON_LESS;
        case DepthTestMode::LessEqual:
            return D3D11_COMPARISON_LESS_EQUAL;
        case DepthTestMode::Greater:
            return D3D11_COMPARISON_GREATER;
        case DepthTestMode::GreaterEqual:
            return D3D11_COMPARISON_GREATER_EQUAL;
        case DepthTestMode::Equal:
            return D3D11_COMPARISON_EQUAL;
        case DepthTestMode::NotEqual:
            return D3D11_COMPARISON_NOT_EQUAL;
        case DepthTestMode::None:
        default:
            return D3D11_COMPARISON_ALWAYS;
        }
    }
}

void Renderer::CreateFallbackVertexBuffers()
{
    // Positionだけは形状を定義できないため補完せず、描画側でエラーとして扱う。
    m_FallbackVertexBuffers[static_cast<size_t>(VertexStreamType::NormalTangent)] = CreateFallbackVertexBuffer(
        m_Device,
        NormalTangentVertex{ Vector3(0.0f, 1.0f, 0.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) });
    m_FallbackVertexBuffers[static_cast<size_t>(VertexStreamType::PrimaryTexCoord)] = CreateFallbackVertexBuffer(
        m_Device,
        PrimaryTexCoordVertex{ Vector4(0.0f, 0.0f, 0.0f, 0.0f) });
    m_FallbackVertexBuffers[static_cast<size_t>(VertexStreamType::SecondaryTexCoord)] = CreateFallbackVertexBuffer(
        m_Device,
        SecondaryTexCoordVertex{ Vector4(0.0f, 0.0f, 0.0f, 0.0f) });
    m_FallbackVertexBuffers[static_cast<size_t>(VertexStreamType::ExtraTexCoord)] = CreateFallbackVertexBuffer(
        m_Device,
        ExtraTexCoordVertex{});
    m_FallbackVertexBuffers[static_cast<size_t>(VertexStreamType::Color)] = CreateFallbackVertexBuffer(
        m_Device,
        ColorVertex{ Color(1.0f, 1.0f, 1.0f, 1.0f) });
}

void Renderer::Init()
{
	HRESULT hr = S_OK;




	// デバイス、スワップチェーン作成
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = SCREEN_WIDTH;
	swapChainDesc.BufferDesc.Height = SCREEN_HEIGHT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetWindow();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain( NULL,
										D3D_DRIVER_TYPE_HARDWARE,
										NULL,
										0,
										NULL,
										0,
										D3D11_SDK_VERSION,
										&swapChainDesc,
										&m_SwapChain,
										&m_Device,
										&m_FeatureLevel,
										&m_DeviceContext );

    m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&m_BackBufferTexture);

    // 実際のバックバッファ設定を取得
    D3D11_TEXTURE2D_DESC backBufferDesc{};
    m_BackBufferTexture->GetDesc(&backBufferDesc);

	// バッファテクスチャ作成
    D3D11_TEXTURE2D_DESC bufferTexDesc{};
    bufferTexDesc.Width = backBufferDesc.Width;
    bufferTexDesc.Height = backBufferDesc.Height;
    bufferTexDesc.MipLevels = 1;
    bufferTexDesc.ArraySize = 1;
    bufferTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    bufferTexDesc.SampleDesc = backBufferDesc.SampleDesc;
    bufferTexDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    for (int i = 0; i < 2; ++i)
    {
        m_Device->CreateTexture2D(&bufferTexDesc, NULL, &m_bufferTexture[i]);
        m_Device->CreateRenderTargetView(m_bufferTexture[i], NULL, &m_bufferRTV[i]);
        m_Device->CreateShaderResourceView(m_bufferTexture[i], NULL, &m_bufferSRV[i]);
    }
	//ID3D11Texture2D* renderTarget{};
	//m_SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&renderTarget );
	//m_Device->CreateRenderTargetView( renderTarget, NULL, &m_RenderTargetView );
	//renderTarget->Release();


	// デプスステンシルバッファ作成
	ID3D11Texture2D* depthStencile{};
	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = swapChainDesc.BufferDesc.Width;
	textureDesc.Height = swapChainDesc.BufferDesc.Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_D16_UNORM;
	textureDesc.SampleDesc = swapChainDesc.SampleDesc;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;
	m_Device->CreateTexture2D(&textureDesc, NULL, &depthStencile);

	// デプスステンシルビュー作成
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = textureDesc.Format;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = 0;
	m_Device->CreateDepthStencilView(depthStencile, &depthStencilViewDesc, &m_DepthStencilView);
	depthStencile->Release();


	m_DeviceContext->OMSetRenderTargets(1, &m_bufferRTV[srcBufferIndex], m_DepthStencilView);


	// ビューポート設定
	D3D11_VIEWPORT viewport;
	viewport.Width = (FLOAT)SCREEN_WIDTH;
	viewport.Height = (FLOAT)SCREEN_HEIGHT;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	m_DeviceContext->RSSetViewports( 1, &viewport );



	// ラスタライザステート設定
	D3D11_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;

    m_Device->CreateRasterizerState(&rasterizerDesc, &m_RasterizerState);

    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    m_Device->CreateRasterizerState(&rasterizerDesc, &m_RasterizerStateNoCull);

    rasterizerDesc.CullMode = D3D11_CULL_FRONT;
    m_Device->CreateRasterizerState(&rasterizerDesc, &m_RasterizerStateFront);

    m_DeviceContext->RSSetState(m_RasterizerState);




	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	m_Device->CreateBlendState( &blendDesc, &m_BlendState );

	blendDesc.AlphaToCoverageEnable = TRUE;
	m_Device->CreateBlendState( &blendDesc, &m_BlendStateATC );

	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	m_Device->CreateBlendState( &blendDesc, &m_BlendStateAdd );

	float blendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff );





    // Shaderで指定できる全14通りを事前生成し、描画中のステート生成をなくす。
    CreateDepthStencilStates();

    // 既存描画と同じLessEqual/書込み有効を初期ステートとして設定する。
    BindRenderState(RenderState{});




	// common.hlsl の共通サンプラーを生成する。
	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	m_Device->CreateSamplerState(&samplerDesc, &m_LinearSampler);
	m_DeviceContext->VSSetSamplers(0, 1, &m_LinearSampler);
	m_DeviceContext->PSSetSamplers(0, 1, &m_LinearSampler);

	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	samplerDesc.BorderColor[0] = 1.0f;
	samplerDesc.BorderColor[1] = 1.0f;
	samplerDesc.BorderColor[2] = 1.0f;
	samplerDesc.BorderColor[3] = 1.0f;
	m_Device->CreateSamplerState(&samplerDesc, &m_ShadowSampler);
	m_DeviceContext->VSSetSamplers(1, 1, &m_ShadowSampler);
	m_DeviceContext->PSSetSamplers(1, 1, &m_ShadowSampler);

    // 任意属性を要求するShaderへ、モデルに存在しない頂点Streamの既定値を供給する。
    CreateFallbackVertexBuffers();



	// common.hlsl の b0〜b3 に対応する定数バッファを生成する。
	D3D11_BUFFER_DESC bufferDesc{};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = sizeof(float);

	bufferDesc.ByteWidth = sizeof(PerFrame);
	m_Device->CreateBuffer(&bufferDesc, nullptr, &m_PerFrameBuffer);
	m_DeviceContext->VSSetConstantBuffers(0, 1, &m_PerFrameBuffer);
	m_DeviceContext->PSSetConstantBuffers(0, 1, &m_PerFrameBuffer);

	bufferDesc.ByteWidth = sizeof(PerObject);
	m_Device->CreateBuffer(&bufferDesc, nullptr, &m_PerObjectBuffer);
	m_DeviceContext->VSSetConstantBuffers(1, 1, &m_PerObjectBuffer);
	m_DeviceContext->PSSetConstantBuffers(1, 1, &m_PerObjectBuffer);

	bufferDesc.ByteWidth = sizeof(LIGHT);
	m_Device->CreateBuffer(&bufferDesc, nullptr, &m_LightingBuffer);
	m_DeviceContext->VSSetConstantBuffers(2, 1, &m_LightingBuffer);
	m_DeviceContext->PSSetConstantBuffers(2, 1, &m_LightingBuffer);

	// b3 は新 Material の Properties として利用する。旧 API は移行完了まで同じスロットを更新する。
	bufferDesc.ByteWidth = sizeof(MATERIAL);
	m_Device->CreateBuffer(&bufferDesc, nullptr, &m_MaterialBuffer);
	m_DeviceContext->VSSetConstantBuffers(3, 1, &m_MaterialBuffer);
	m_DeviceContext->PSSetConstantBuffers(3, 1, &m_MaterialBuffer);

    bufferDesc.ByteWidth = sizeof(XMFLOAT4);
    m_Device->CreateBuffer(&bufferDesc, NULL, &m_UVBuffer);
    m_DeviceContext->VSSetConstantBuffers(6, 1, &m_UVBuffer);
    m_DeviceContext->PSSetConstantBuffers(6, 1, &m_UVBuffer);



	// 行列は単位行列で初期化し、初回描画前に不定値を渡さないようにする。
	SetWorldMatrix(XMMatrixIdentity());
	SetViewMatrix(XMMatrixIdentity());
	SetProjectionMatrix(XMMatrixIdentity());

	// ライト初期化
	LIGHT light{};
	light.Enable = true;
	light.Direction = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	light.Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	light.Diffuse = XMFLOAT4(1.5f, 1.5f, 1.5f, 1.0f);
	SetLight(light);



	// マテリアル初期化
	MATERIAL material{};
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	SetMaterial(material);

    // UV初期化
    SetUV(XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f));


}



void Renderer::Uninit()
{

	m_PerFrameBuffer->Release();
	m_PerObjectBuffer->Release();
	m_LightingBuffer->Release();
	m_MaterialBuffer->Release();
    m_UVBuffer->Release();

    for (ID3D11Buffer* fallbackVertexBuffer : m_FallbackVertexBuffers)
    {
        if (fallbackVertexBuffer)
            fallbackVertexBuffer->Release();
    }
    m_FallbackVertexBuffers = {};

	m_LinearSampler->Release();
	m_ShadowSampler->Release();

    for (ID3D11DepthStencilState* depthState : m_DepthStates)
    {
        if (depthState)
            depthState->Release();
    }

    m_RasterizerState->Release();
    m_RasterizerStateNoCull->Release();
    m_RasterizerStateFront->Release();


	m_DeviceContext->ClearState();
	m_BackBufferTexture->Release();
	m_SwapChain->Release();
	m_DeviceContext->Release();
	m_Device->Release();

    for (int i = 0; i < 2; ++i)
    {
        m_bufferTexture[i]->Release();
        m_bufferRTV[i]->Release();
        m_bufferSRV[i]->Release();
    }
}




void Renderer::Begin()
{
	float clearColor[4] = {
		0 / 255.0f,
		0 / 255.0f,
		16.0f / 255.0f,
		1.0f
	};
    for( int i = 0; i < 2; ++i )
    {
        m_DeviceContext->ClearRenderTargetView(m_bufferRTV[i], clearColor );
    }
	m_DeviceContext->ClearDepthStencilView( m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Renderer::Draw()
{
    for (size_t phaseIndex = 0; phaseIndex < m_DrawQueues.size(); ++phaseIndex)
    {
        const RenderPhase phase = static_cast<RenderPhase>(phaseIndex);
        if (phase == RenderPhase::PostProcess)
            continue;

        auto& queue = m_DrawQueues[phaseIndex];
        std::sort(queue.begin(), queue.end(), [](const DrawItem& a, const DrawItem& b)
        {
            if (a.sortKey != b.sortKey)
                return a.sortKey < b.sortKey;
            return a.submitOrder < b.submitOrder;
        });

        // 互換経路は旧 RendererBase と同じ深度動作を維持する。
        if (phase == RenderPhase::Opaque)
            SetDepthEnable(true);
        else if (phase == RenderPhase::Transparent)
            SetDepthEnable(false);

        for (const DrawItem& item : queue)
        {
            if (item.material)
            {
                // Materialが所有するShaderとPassの描画設定を、描画直前に一括で反映する。
                item.material->Bind(item.materialPassIndex);
            }
            else if (item.renderer)
            {
                // Material未移行のRendererは、従来どおり個別ステートを設定する。
                item.renderer->BindState();
            }

            if (item.renderer)
            {
                item.renderer->Draw();
            }
        }

        queue.clear();
    }
}

bool Renderer::SetRenderTarget(TextureHandle renderTexture, bool useDepthStencil)
{
    ID3D11RenderTargetView* renderTargetView = TextureManager::GetRenderTargetView(renderTexture);
    UINT width{};
    UINT height{};
    if (!renderTargetView || !TextureManager::GetTextureSize(renderTexture, width, height))
    {
        MessageBoxA(nullptr, "指定されたTextureHandleはRenderTextureではありません。", "Renderer Error", MB_OK | MB_ICONERROR);
        return false;
    }

    // RenderTextureごとのRTV解決をRendererへ集約し、呼び出し側はHandleだけを扱う。
    m_DeviceContext->OMSetRenderTargets(1, &renderTargetView, useDepthStencil ? m_DepthStencilView : nullptr);

    // RT全体を描画対象にする標準Viewportを同時に設定する。
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_DeviceContext->RSSetViewports(1, &viewport);

    return true;
}

void Renderer::SetDstRTV()
{
    m_DeviceContext->OMSetRenderTargets(1, &m_bufferRTV[dstBufferIndex], m_DepthStencilView);

    // Renderer所有の画面バッファへ戻す場合も、RT全体のViewportへ復帰する。
    D3D11_TEXTURE2D_DESC description{};
    m_bufferTexture[dstBufferIndex]->GetDesc(&description);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(description.Width);
    viewport.Height = static_cast<float>(description.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_DeviceContext->RSSetViewports(1, &viewport);
}



void Renderer::End()
{
    // RTVのデータをバックバッファにコピーする
    m_DeviceContext->CopyResource(m_BackBufferTexture, m_bufferTexture[srcBufferIndex]);

    // バッファを入れ替える
	m_SwapChain->Present( 1, 0 );
}




void Renderer::SetDepthEnable( bool Enable )
{
    // 旧APIのfalseは、深度テストを残して書込みだけを止める従来挙動を維持する。
    const DepthWriteMode writeMode = Enable ? DepthWriteMode::Write : DepthWriteMode::None;
    BindRenderState({ RenderPhase::Opaque, m_CurrentBlendMode, m_CurrentCullMode, DepthTestMode::LessEqual, writeMode });

}



void Renderer::SetATCEnable( bool Enable )
{
	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if (Enable)
		m_DeviceContext->OMSetBlendState(m_BlendStateATC, blendFactor, 0xffffffff);
	else
		m_DeviceContext->OMSetBlendState(m_BlendState, blendFactor, 0xffffffff);

}

void Renderer::SetWorldViewProjection2D()
{
	SetWorldMatrix(XMMatrixIdentity());
	SetViewMatrix(XMMatrixIdentity());

	XMMATRIX projection;
	projection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);
	SetProjectionMatrix(projection);
}


void Renderer::SetWorldMatrix(XMMATRIX WorldMatrix)
{
	XMStoreFloat4x4(&m_PerObject.World, XMMatrixTranspose(WorldMatrix));
	m_DeviceContext->UpdateSubresource(m_PerObjectBuffer, 0, nullptr, &m_PerObject, 0, 0);
}

void Renderer::SetViewMatrix(XMMATRIX ViewMatrix)
{
    m_ViewMatrix = ViewMatrix;
	XMStoreFloat4x4(&m_PerFrame.View, XMMatrixTranspose(ViewMatrix));
	m_DeviceContext->UpdateSubresource(m_PerFrameBuffer, 0, nullptr, &m_PerFrame, 0, 0);
}

void Renderer::SetProjectionMatrix(XMMATRIX ProjectionMatrix)
{
    m_ProjectionMatrix = ProjectionMatrix;
	XMStoreFloat4x4(&m_PerFrame.Projection, XMMatrixTranspose(ProjectionMatrix));
	m_DeviceContext->UpdateSubresource(m_PerFrameBuffer, 0, nullptr, &m_PerFrame, 0, 0);

}



void Renderer::SetMaterial( MATERIAL Material )
{
	m_DeviceContext->UpdateSubresource( m_MaterialBuffer, 0, NULL, &Material, 0, 0 );
}

void Renderer::SetLight( LIGHT Light )
{
	m_DeviceContext->UpdateSubresource(m_LightingBuffer, 0, nullptr, &Light, 0, 0);
}

void Renderer::SetUV(XMFLOAT4 UV)
{
    m_DeviceContext->UpdateSubresource(m_UVBuffer, 0, NULL, &UV, 0, 0);
}

ID3D11Buffer* Renderer::GetFallbackVertexBuffer(VertexStreamType streamType)
{
    const size_t index = static_cast<size_t>(streamType);
    if (streamType == VertexStreamType::Position || streamType == VertexStreamType::Error ||
        index >= m_FallbackVertexBuffers.size())
    {
        return nullptr;
    }

    return m_FallbackVertexBuffers[index];
}

void Renderer::SetBlendMode(BlendMode mode)
{
    switch (mode)
    {
    case BlendMode::AlphaBlend:
        m_DeviceContext->OMSetBlendState(m_BlendState, nullptr, 0xffffffff);
        break;

    case BlendMode::Add:
        m_DeviceContext->OMSetBlendState(m_BlendStateAdd, nullptr, 0xffffffff);
        break;

    case BlendMode::None:
        m_DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        break;
    default:
        break;
    }

    m_CurrentBlendMode = mode;
}

void Renderer::SetRasterizerState(CullMode cull)
{
    switch (cull)
    {
    case CullMode::None:
        m_DeviceContext->RSSetState(m_RasterizerStateNoCull);
        break;
    case CullMode::Back:
        m_DeviceContext->RSSetState(m_RasterizerState);
        break;
    case CullMode::Front:
        m_DeviceContext->RSSetState(m_RasterizerStateFront);
        break;
    default:
        break;
    }
    m_CurrentCullMode = cull;
}

void Renderer::BindRenderState(const RenderState& renderState)
{
    SetBlendMode(renderState.blendMode);
    SetRasterizerState(renderState.cullMode);

    ID3D11DepthStencilState* depthState = GetDepthStencilState(
        renderState.depthTestMode,
        renderState.depthWriteMode);
    m_DeviceContext->OMSetDepthStencilState(depthState, 0);
}

ID3D11DepthStencilState* Renderer::GetDepthStencilState(
    DepthTestMode testMode,
    DepthWriteMode writeMode)
{
    const size_t stateIndex = GetDepthStateIndex(testMode, writeMode);
    return m_DepthStates[stateIndex];
}

void Renderer::CreateDepthStencilStates()
{
    for (size_t testMode = 0; testMode < static_cast<size_t>(DepthTestMode::Count); ++testMode)
    {
        for (size_t writeMode = 0; writeMode < static_cast<size_t>(DepthWriteMode::Count); ++writeMode)
        {
            const DepthTestMode currentTestMode = static_cast<DepthTestMode>(testMode);
            const DepthWriteMode currentWriteMode = static_cast<DepthWriteMode>(writeMode);

            D3D11_DEPTH_STENCIL_DESC description{};
            description.DepthEnable = currentTestMode != DepthTestMode::None;
            description.DepthWriteMask = currentWriteMode == DepthWriteMode::Write
                ? D3D11_DEPTH_WRITE_MASK_ALL
                : D3D11_DEPTH_WRITE_MASK_ZERO;
            description.DepthFunc = ToD3DComparisonFunc(currentTestMode);
            description.StencilEnable = FALSE;

            // 生成したStateは列挙値から決まる固定添字へ保存し、Bind時は再生成しない。
            const size_t stateIndex = GetDepthStateIndex(currentTestMode, currentWriteMode);
            m_Device->CreateDepthStencilState(&description, &m_DepthStates[stateIndex]);
        }
    }
}

XMMATRIX Renderer::GetViewMatrix()
{
    return m_ViewMatrix;
}

XMMATRIX Renderer::GetProjectionMatrix()
{
    return m_ProjectionMatrix;
}

void Renderer::CreateVertexShader( ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName )
{

	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	m_Device->CreateVertexShader(buffer, fsize, NULL, VertexShader);


	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 10, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	m_Device->CreateInputLayout(layout,
		numElements,
		buffer,
		fsize,
		VertexLayout);

	delete[] buffer;
}



void Renderer::CreatePixelShader( ID3D11PixelShader** PixelShader, const char* FileName )
{
	FILE* file;
	long int fsize;

	file = fopen(FileName, "rb");
	assert(file);

	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	m_Device->CreatePixelShader(buffer, fsize, NULL, PixelShader);

	delete[] buffer;
}

void Renderer::SubmitDrawItem(RenderPhase phase, DrawItem item)
{
    if (phase == RenderPhase::PostProcess || phase >= RenderPhase::Count)
        return;

    item.submitOrder = m_SubmitOrder++;
    m_DrawQueues[static_cast<size_t>(phase)].push_back(item);
}

void Renderer::SubmitDrawCommand(RenderPhase phase, DrawItem item)
{
    SubmitDrawItem(phase, item);
}
