#include "pch.h"

#include "System/Core/main.h"
#include "System/Core/GM_Time.h"
#include "System/Rendering/renderer.h"
#include "System/Rendering/RendererBase.h"
#include "System/Core/GameObject.h"
#include "Component/Camera.h"
#include "Component/PostEffect.h"
#include "Graphics/Material.h"
#include "Graphics/Shader/ShaderProgram.h"
#include "Graphics/TextureManager.h"
#include <io.h>


D3D_FEATURE_LEVEL       Renderer::m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*           Renderer::m_Device{};
ID3D11DeviceContext*    Renderer::m_DeviceContext{};
IDXGISwapChain*         Renderer::m_SwapChain{};
ID3D11Texture2D*        Renderer::m_BackBufferTexture{};
ID3D11RenderTargetView* Renderer::m_BackBufferRTV{};
ID3D11DepthStencilView* Renderer::m_DepthStencilView{};

ID3D11Texture2D*            Renderer::m_bufferTexture[2]{};
ID3D11RenderTargetView*     Renderer::m_bufferRTV[2]{};
ID3D11ShaderResourceView*   Renderer::m_bufferSRV[2]{};
UINT Renderer::m_gameWidth{};
UINT Renderer::m_gameHeight{};
int Renderer::srcBufferIndex = 0;
int Renderer::dstBufferIndex = 1;

ID3D11Buffer*			Renderer::m_PerFrameBuffer{};
ID3D11Buffer*			Renderer::m_PerObjectBuffer{};
ID3D11Buffer*			Renderer::m_LightingBuffer{};
ID3D11Buffer*            Renderer::m_ShadowParametersBuffer{};
std::array<ID3D11Buffer*, static_cast<size_t>(VertexStreamType::Count)> Renderer::m_FallbackVertexBuffers{};


std::array<ID3D11DepthStencilState*,
    static_cast<size_t>(DepthTestMode::Count) * static_cast<size_t>(DepthWriteMode::Count)>
    Renderer::m_DepthStates{};

ID3D11RasterizerState* Renderer::m_RasterizerState{};
ID3D11RasterizerState* Renderer::m_RasterizerStateNoCull{};
ID3D11RasterizerState* Renderer::m_RasterizerStateFront{};
ID3D11RasterizerState* Renderer::m_ShadowRasterizerState{};


ID3D11BlendState*		Renderer::m_BlendState{};
ID3D11BlendState*       Renderer::m_BlendStateAdd{};
ID3D11BlendState*		Renderer::m_BlendStateATC{};
ID3D11SamplerState*		Renderer::m_LinearSampler{};
ID3D11SamplerState*		Renderer::m_ClampSampler{};
ID3D11SamplerState*		Renderer::m_ShadowSampler{};
ID3D11Texture2D* Renderer::m_ShadowMapTexture{};
std::array<ID3D11DepthStencilView*, SHADOW_CASCADE_COUNT> Renderer::m_ShadowMapDSVs{};
ID3D11ShaderResourceView* Renderer::m_ShadowMapSRV{};

XMMATRIX Renderer::m_ViewMatrix{};
XMMATRIX Renderer::m_ProjectionMatrix{};
PerFrame Renderer::m_PerFrame{};
PerObject Renderer::m_PerObject{};
ShadowParameters Renderer::m_ShadowParameters{};
LIGHT Renderer::m_Light{};
RenderPhase Renderer::m_CurrentRenderPhase{ RenderPhase::Opaque };

std::array<std::vector<DrawItem>, static_cast<size_t>(RenderPhase::Count)> Renderer::m_DrawQueues{};
std::vector<PostProcess*> Renderer::m_postEffects{};
uint64_t Renderer::m_SubmitOrder{};
BlendMode Renderer::m_CurrentBlendMode{ BlendMode::AlphaBlend };
CullMode Renderer::m_CurrentCullMode{ CullMode::Back };

namespace
{
    // 再生成を行わないため、一般的なモデルの頂点数を十分に上回る2^20頂点を確保する。
    constexpr UINT FALLBACK_VERTEX_COUNT = 1u << 20;
    constexpr UINT SHADOW_MAP_SIZE = 2048;
    constexpr float CAMERA_NEAR_DISTANCE = 0.1f;
    constexpr float CAMERA_FAR_DISTANCE = 1000.0f;
    constexpr std::array<float, SHADOW_CASCADE_COUNT + 1> SHADOW_CASCADE_DISTANCES
    {
        CAMERA_NEAR_DISTANCE, 10.0f, 25.0f, 55.0f, 100.0f,
    };

    bool IsPostProcessPhase(RenderPhase phase)
    {
        return phase == RenderPhase::PostProcess || phase == RenderPhase::FinalPostProcess;
    }

    bool IsPostEffectInPhase(const PostProcess& postEffect, RenderPhase phase)
    {
        const Material* material = postEffect.GetMaterial();
        if (!material || material->GetShader().GetShaderPasses().empty())
        {
            return false;
        }

        return material->GetShaderPass(0).GetRenderState().renderPhase == phase;
    }

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

bool Renderer::CreateShadowMap()
{
    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width = SHADOW_MAP_SIZE;
    textureDesc.Height = SHADOW_MAP_SIZE;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = SHADOW_CASCADE_COUNT;
    textureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(m_Device->CreateTexture2D(&textureDesc, nullptr, &m_ShadowMapTexture)))
        return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    dsvDesc.Texture2DArray.ArraySize = 1;
    for (size_t cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_COUNT; ++cascadeIndex)
    {
        dsvDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(cascadeIndex);
        if (FAILED(m_Device->CreateDepthStencilView(m_ShadowMapTexture, &dsvDesc, &m_ShadowMapDSVs[cascadeIndex])))
            return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MipLevels = 1;
    srvDesc.Texture2DArray.ArraySize = SHADOW_CASCADE_COUNT;
    return SUCCEEDED(m_Device->CreateShaderResourceView(m_ShadowMapTexture, &srvDesc, &m_ShadowMapSRV));
}

void Renderer::UpdateShadowParameters()
{
    const XMMATRIX inverseViewProjection = XMMatrixInverse(nullptr, m_ViewMatrix * m_ProjectionMatrix);
    XMVECTOR frustumCorners[8]{};
    size_t index = 0;
    for (float z : { 0.0f, 1.0f })
    {
        for (float y : { -1.0f, 1.0f })
        {
            for (float x : { -1.0f, 1.0f })
            {
                frustumCorners[index++] = XMVector3TransformCoord(XMVectorSet(x, y, z, 1.0f), inverseViewProjection);
            }
        }
    }

    XMVECTOR direction = XMVector3Normalize(XMLoadFloat4(&m_Light.Direction));
    if (XMVectorGetX(XMVector3LengthSq(direction)) < 0.0001f)
        direction = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

    const XMVECTOR up = abs(XMVectorGetY(direction)) > 0.99f
        ? XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f)
        : XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    for (size_t cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_COUNT; ++cascadeIndex)
    {
        const float nearRatio = (SHADOW_CASCADE_DISTANCES[cascadeIndex] - CAMERA_NEAR_DISTANCE) /
            (CAMERA_FAR_DISTANCE - CAMERA_NEAR_DISTANCE);
        const float farRatio = (SHADOW_CASCADE_DISTANCES[cascadeIndex + 1] - CAMERA_NEAR_DISTANCE) /
            (CAMERA_FAR_DISTANCE - CAMERA_NEAR_DISTANCE);
        XMVECTOR corners[8]{};
        XMVECTOR center = XMVectorZero();
        for (size_t cornerIndex = 0; cornerIndex < 4; ++cornerIndex)
        {
            corners[cornerIndex] = XMVectorLerp(frustumCorners[cornerIndex], frustumCorners[cornerIndex + 4], nearRatio);
            corners[cornerIndex + 4] = XMVectorLerp(frustumCorners[cornerIndex], frustumCorners[cornerIndex + 4], farRatio);
            center += corners[cornerIndex] + corners[cornerIndex + 4];
        }
        center /= 8.0f;

        const XMMATRIX lightView = XMMatrixLookAtLH(center - direction * 100.0f, center, up);
        XMVECTOR minPoint = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        XMVECTOR maxPoint = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);
        for (const XMVECTOR corner : corners)
        {
            const XMVECTOR lightSpace = XMVector3TransformCoord(corner, lightView);
            minPoint = XMVectorMin(minPoint, lightSpace);
            maxPoint = XMVectorMax(maxPoint, lightSpace);
        }

        const float width = XMVectorGetX(maxPoint) - XMVectorGetX(minPoint);
        const float height = XMVectorGetY(maxPoint) - XMVectorGetY(minPoint);
        const float texelWidth = width / SHADOW_MAP_SIZE;
        const float texelHeight = height / SHADOW_MAP_SIZE;
        const float centerX = floorf((XMVectorGetX(minPoint) + XMVectorGetX(maxPoint)) * 0.5f / texelWidth) * texelWidth;
        const float centerY = floorf((XMVectorGetY(minPoint) + XMVectorGetY(maxPoint)) * 0.5f / texelHeight) * texelHeight;
        const float minZ = XMVectorGetZ(minPoint) - 100.0f;
        const float maxZ = XMVectorGetZ(maxPoint) + 100.0f;
        const XMMATRIX lightProjection = XMMatrixOrthographicOffCenterLH(
            centerX - width * 0.5f, centerX + width * 0.5f,
            centerY - height * 0.5f, centerY + height * 0.5f,
            minZ, maxZ);
        XMStoreFloat4x4(&m_ShadowParameters.LightViewProjection[cascadeIndex], XMMatrixTranspose(lightView * lightProjection));
    }

    m_ShadowParameters.ShadowMapTexelSize = XMFLOAT2(1.0f / SHADOW_MAP_SIZE, 1.0f / SHADOW_MAP_SIZE);
    m_ShadowParameters.CascadeFarDistances = XMFLOAT4(
        SHADOW_CASCADE_DISTANCES[1], SHADOW_CASCADE_DISTANCES[2],
        SHADOW_CASCADE_DISTANCES[3], SHADOW_CASCADE_DISTANCES[4]);
}

void Renderer::BeginShadowPass(size_t cascadeIndex)
{
    ID3D11ShaderResourceView* nullSRV{};
    m_DeviceContext->PSSetShaderResources(1, 1, &nullSRV);
    m_ShadowParameters.CurrentCascadeIndex = static_cast<UINT>(cascadeIndex);
    m_DeviceContext->UpdateSubresource(m_ShadowParametersBuffer, 0, nullptr, &m_ShadowParameters, 0, 0);
    m_DeviceContext->OMSetRenderTargets(0, nullptr, m_ShadowMapDSVs[cascadeIndex]);
    m_DeviceContext->ClearDepthStencilView(m_ShadowMapDSVs[cascadeIndex], D3D11_CLEAR_DEPTH, 1.0f, 0);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.Height = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_DeviceContext->RSSetViewports(1, &viewport);
}

void Renderer::EndShadowPass()
{
    SetSrcRTV();
    m_DeviceContext->PSSetShaderResources(1, 1, &m_ShadowMapSRV);
}

bool Renderer::CreateGameRenderTargets(UINT width, UINT height)
{
    if (!m_Device || !m_BackBufferTexture || width == 0 || height == 0)
        return false;

    D3D11_TEXTURE2D_DESC backBufferDescription{};
    m_BackBufferTexture->GetDesc(&backBufferDescription);

    D3D11_TEXTURE2D_DESC renderTargetDescription{};
    renderTargetDescription.Width = width;
    renderTargetDescription.Height = height;
    renderTargetDescription.MipLevels = 1;
    renderTargetDescription.ArraySize = 1;
    renderTargetDescription.Format = backBufferDescription.Format;
    renderTargetDescription.SampleDesc = backBufferDescription.SampleDesc;
    renderTargetDescription.Usage = D3D11_USAGE_DEFAULT;
    renderTargetDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> textures[2];
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetViews[2];
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceViews[2];
    for (int i = 0; i < 2; ++i)
    {
        if (FAILED(m_Device->CreateTexture2D(&renderTargetDescription, nullptr, textures[i].GetAddressOf())) ||
            FAILED(m_Device->CreateRenderTargetView(textures[i].Get(), nullptr, renderTargetViews[i].GetAddressOf())) ||
            FAILED(m_Device->CreateShaderResourceView(textures[i].Get(), nullptr, shaderResourceViews[i].GetAddressOf())))
        {
            return false;
        }
    }

    D3D11_TEXTURE2D_DESC depthDescription{};
    depthDescription.Width = width;
    depthDescription.Height = height;
    depthDescription.MipLevels = 1;
    depthDescription.ArraySize = 1;
    depthDescription.Format = DXGI_FORMAT_D16_UNORM;
    depthDescription.SampleDesc = renderTargetDescription.SampleDesc;
    depthDescription.Usage = D3D11_USAGE_DEFAULT;
    depthDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthTexture;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
    if (FAILED(m_Device->CreateTexture2D(&depthDescription, nullptr, depthTexture.GetAddressOf())))
        return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC depthViewDescription{};
    depthViewDescription.Format = depthDescription.Format;
    depthViewDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    if (FAILED(m_Device->CreateDepthStencilView(
        depthTexture.Get(), &depthViewDescription, depthStencilView.GetAddressOf())))
    {
        return false;
    }

    ReleaseGameRenderTargets();
    for (int i = 0; i < 2; ++i)
    {
        m_bufferTexture[i] = textures[i].Detach();
        m_bufferRTV[i] = renderTargetViews[i].Detach();
        m_bufferSRV[i] = shaderResourceViews[i].Detach();
    }
    m_DepthStencilView = depthStencilView.Detach();
    m_gameWidth = width;
    m_gameHeight = height;
    srcBufferIndex = 0;
    dstBufferIndex = 1;
    return true;
}

void Renderer::ReleaseGameRenderTargets()
{
    for (int i = 0; i < 2; ++i)
    {
        if (m_bufferSRV[i])
            m_bufferSRV[i]->Release();
        if (m_bufferRTV[i])
            m_bufferRTV[i]->Release();
        if (m_bufferTexture[i])
            m_bufferTexture[i]->Release();

        m_bufferSRV[i] = nullptr;
        m_bufferRTV[i] = nullptr;
        m_bufferTexture[i] = nullptr;
    }

    if (m_DepthStencilView)
        m_DepthStencilView->Release();
    m_DepthStencilView = nullptr;
    m_gameWidth = 0;
    m_gameHeight = 0;
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
	RECT clientRect{};
	GetClientRect(GetWindow(), &clientRect);
	const UINT clientWidth = clientRect.right > clientRect.left
		? static_cast<UINT>(clientRect.right - clientRect.left)
		: SCREEN_WIDTH;
	const UINT clientHeight = clientRect.bottom > clientRect.top
		? static_cast<UINT>(clientRect.bottom - clientRect.top)
		: SCREEN_HEIGHT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = clientWidth;
	swapChainDesc.BufferDesc.Height = clientHeight;
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

	// ImGui と Runtime の最終出力先になる BackBuffer の RTV を作成する。
    if (FAILED(m_Device->CreateRenderTargetView(m_BackBufferTexture, nullptr, &m_BackBufferRTV)) ||
        !CreateGameRenderTargets(backBufferDesc.Width, backBufferDesc.Height) || !CreateShadowMap())
    {
        return;
    }


	m_DeviceContext->OMSetRenderTargets(1, &m_bufferRTV[srcBufferIndex], m_DepthStencilView);


	// 初期描画先とビューポートを Game RenderTarget に設定する。
	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<FLOAT>(m_gameWidth);
	viewport.Height = static_cast<FLOAT>(m_gameHeight);
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

    // 固定Biasを主にし、傾斜面だけSlope Biasで補う。接地影の浮きを抑えるため各値は小さく保つ。
    rasterizerDesc.DepthBias = 100;
    rasterizerDesc.SlopeScaledDepthBias = 0.25f;
    rasterizerDesc.DepthBiasClamp = 0.0001f;
    m_Device->CreateRasterizerState(&rasterizerDesc, &m_ShadowRasterizerState);

    m_DeviceContext->RSSetState(m_RasterizerState);




	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	// Game RenderTextureは最終的に画面全体として表示するため、色の合成後も出力Alphaは背景を保持する。
	// これにより、EditorのImGui::ImageがSpriteの透過部分でシーン全体を抜かない。
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
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

	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_Device->CreateSamplerState(&samplerDesc, &m_ClampSampler);
	m_DeviceContext->VSSetSamplers(2, 1, &m_ClampSampler);
	m_DeviceContext->PSSetSamplers(2, 1, &m_ClampSampler);

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



	// common.hlsl の共有定数バッファを生成する。
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

    bufferDesc.ByteWidth = sizeof(ShadowParameters);
    m_Device->CreateBuffer(&bufferDesc, nullptr, &m_ShadowParametersBuffer);
    m_DeviceContext->VSSetConstantBuffers(4, 1, &m_ShadowParametersBuffer);
    m_DeviceContext->PSSetConstantBuffers(4, 1, &m_ShadowParametersBuffer);


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

	PostProcess::Init();


}



void Renderer::Uninit()
{
	ClearPostEffects();
	PostProcess::Uninit();


	m_PerFrameBuffer->Release();
	m_PerObjectBuffer->Release();
	m_LightingBuffer->Release();
	if (m_ShadowParametersBuffer)
        m_ShadowParametersBuffer->Release();
	if (m_ShadowMapSRV)
        m_ShadowMapSRV->Release();
	for (ID3D11DepthStencilView* shadowMapDSV : m_ShadowMapDSVs)
    {
        if (shadowMapDSV)
            shadowMapDSV->Release();
    }
    m_ShadowMapDSVs = {};
	if (m_ShadowMapTexture)
        m_ShadowMapTexture->Release();

    for (ID3D11Buffer* fallbackVertexBuffer : m_FallbackVertexBuffers)
    {
        if (fallbackVertexBuffer)
            fallbackVertexBuffer->Release();
    }
    m_FallbackVertexBuffers = {};

	m_LinearSampler->Release();
	m_ClampSampler->Release();
	m_ShadowSampler->Release();
	if (m_ShadowRasterizerState)
        m_ShadowRasterizerState->Release();

    for (ID3D11DepthStencilState* depthState : m_DepthStates)
    {
        if (depthState)
            depthState->Release();
    }

	m_RasterizerState->Release();
	m_RasterizerStateNoCull->Release();
	m_RasterizerStateFront->Release();


	m_DeviceContext->ClearState();
	ReleaseGameRenderTargets();
	if (m_BackBufferRTV)
	{
		m_BackBufferRTV->Release();
		m_BackBufferRTV = nullptr;
	}
	m_BackBufferTexture->Release();
	m_BackBufferTexture = nullptr;
	m_SwapChain->Release();
	m_SwapChain = nullptr;
	m_DeviceContext->Release();
	m_DeviceContext = nullptr;
	m_Device->Release();
	m_Device = nullptr;
}




void Renderer::Begin()
{
	// PerFrameを毎フレーム更新し、common.hlslのTimeへゲーム経過時間を渡す。
	m_PerFrame.Time = Time::GetTotalTime();
	m_DeviceContext->UpdateSubresource(m_PerFrameBuffer, 0, nullptr, &m_PerFrame, 0, 0);

	// ImGuiの外部Viewport描画がD3D11コンテキストのRTVを変更するため、毎フレームの描画先を明示的に復元する。
	m_DeviceContext->OMSetRenderTargets(1, &m_bufferRTV[srcBufferIndex], m_DepthStencilView);

	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(m_gameWidth);
	viewport.Height = static_cast<float>(m_gameHeight);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_DeviceContext->RSSetViewports(1, &viewport);

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
        m_CurrentRenderPhase = phase;

        if (IsPostProcessPhase(phase))
        {
            for (PostProcess* postEffect : m_postEffects)
            {
                if (postEffect && IsPostEffectInPhase(*postEffect, phase))
                {
                    postEffect->Apply();
                }
            }

            // 次の描画Phaseは、完成済みの画面バッファへ重ねる。
            SetSrcRTV();
            continue;
        }

        auto& queue = m_DrawQueues[phaseIndex];
        std::sort(queue.begin(), queue.end(), [](const DrawItem& a, const DrawItem& b)
        {
            if (a.sortKey != b.sortKey)
                return a.sortKey < b.sortKey;
            return a.submitOrder < b.submitOrder;
        });

        if (phase == RenderPhase::Shadow)
        {
            UpdateShadowParameters();
            for (size_t cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_COUNT; ++cascadeIndex)
            {
                BeginShadowPass(cascadeIndex);
                for (const DrawItem& item : queue)
                {
                    if (item.renderer)
                        item.renderer->Draw();
                }
            }
        }
        else
        {
            for (const DrawItem& item : queue)
            {
                if (item.renderer)
                    item.renderer->Draw();
            }
        }

        queue.clear();

        if (phase == RenderPhase::Shadow)
            EndShadowPass();
    }
}

bool Renderer::RegisterPostEffect(PostProcess* postEffect)
{
    if (!postEffect || std::find(m_postEffects.begin(), m_postEffects.end(), postEffect) != m_postEffects.end())
        return false;

    m_postEffects.push_back(postEffect);
    return true;
}

void Renderer::UnregisterPostEffect(PostProcess* postEffect)
{
    const auto iterator = std::find(m_postEffects.begin(), m_postEffects.end(), postEffect);
    if (iterator != m_postEffects.end())
        m_postEffects.erase(iterator);
}

PostProcess* Renderer::FindPostEffect(std::string_view name)
{
    for (PostProcess* postEffect : m_postEffects)
    {
        if (postEffect && postEffect->GetName() == name)
            return postEffect;
    }

    return nullptr;
}

bool Renderer::IsPostProcessMaterialUsed(const Material* material, const PostProcess* excludedPostEffect)
{
    if (!material)
        return false;

    for (const PostProcess* postEffect : m_postEffects)
    {
        if (postEffect != excludedPostEffect && postEffect && postEffect->GetMaterial() == material)
            return true;
    }

    return false;
}

void Renderer::ClearPostEffects()
{
    m_postEffects.clear();
}

void Renderer::ClearScenePostEffects()
{
    std::erase_if(m_postEffects, [](PostProcess* postEffect)
    {
        return !postEffect
            || !postEffect->GetOwner()
            || !postEffect->GetOwner()->IsNotDestroyBySceneEnd();
    });
}

void Renderer::ResizePostEffects()
{
    for (PostProcess* postEffect : m_postEffects)
    {
        if (postEffect)
            postEffect->Resize(m_gameWidth, m_gameHeight);
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
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(m_gameWidth);
	viewport.Height = static_cast<float>(m_gameHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
	m_DeviceContext->RSSetViewports(1, &viewport);
}

void Renderer::SetSrcRTV()
{
    m_DeviceContext->OMSetRenderTargets(1, &m_bufferRTV[srcBufferIndex], m_DepthStencilView);

    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(m_gameWidth);
    viewport.Height = static_cast<float>(m_gameHeight);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_DeviceContext->RSSetViewports(1, &viewport);
}


bool Renderer::ResizeBackBuffer(UINT width, UINT height)
{
    width = std::max<UINT>(width, 1);
    height = std::max<UINT>(height, 1);
    if (!m_SwapChain || !m_Device || !m_DeviceContext)
        return false;

    D3D11_TEXTURE2D_DESC currentDescription{};
    if (m_BackBufferTexture)
        m_BackBufferTexture->GetDesc(&currentDescription);
    if (currentDescription.Width == width && currentDescription.Height == height)
        return true;

    // SwapChainのResizeBuffers前に、BackBufferとGame RTの参照を解除する。
    // ClearStateは共通定数バッファやSamplerまで解除してしまうため、Resize後の描画状態を壊さない。
    m_DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView* nullResources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};
    m_DeviceContext->VSSetShaderResources(
        0,
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
        nullResources);
    m_DeviceContext->PSSetShaderResources(
        0,
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
        nullResources);
    m_DeviceContext->Flush();
    ReleaseGameRenderTargets();
    if (m_BackBufferRTV)
    {
        m_BackBufferRTV->Release();
        m_BackBufferRTV = nullptr;
    }
    if (m_BackBufferTexture)
    {
        m_BackBufferTexture->Release();
        m_BackBufferTexture = nullptr;
    }

    if (FAILED(m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)) ||
        FAILED(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_BackBufferTexture))) ||
        FAILED(m_Device->CreateRenderTargetView(m_BackBufferTexture, nullptr, &m_BackBufferRTV)) ||
        !CreateGameRenderTargets(width, height))
    {
        return false;
    }

    m_DeviceContext->OMSetRenderTargets(1, &m_bufferRTV[srcBufferIndex], m_DepthStencilView);
    return true;
}


bool Renderer::ResizeGameRenderTarget(UINT width, UINT height)
{
	width = std::max<UINT>(width, 1);
	height = std::max<UINT>(height, 1);
	if (width == m_gameWidth && height == m_gameHeight)
		return true;

	return CreateGameRenderTargets(width, height);
}

void Renderer::BeginEditorFrame()
{
	if (!m_DeviceContext || !m_BackBufferRTV)
		return;

	// Game RT から切り替えた後、ImGui は BackBuffer へ描画する。
	m_DeviceContext->OMSetRenderTargets(1, &m_BackBufferRTV, nullptr);

	D3D11_TEXTURE2D_DESC description{};
	m_BackBufferTexture->GetDesc(&description);
	D3D11_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(description.Width);
	viewport.Height = static_cast<float>(description.Height);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_DeviceContext->RSSetViewports(1, &viewport);

	const float clearColor[4] = { 0.0f, 0.0f, 16.0f / 255.0f, 1.0f };
	m_DeviceContext->ClearRenderTargetView(m_BackBufferRTV, clearColor);
}



void Renderer::End()
{
#if !GM_ENABLE_EDITOR
    // Runtime は ImGui を介さず、完成した Game RenderTexture を BackBuffer へコピーする。
    m_DeviceContext->CopyResource(m_BackBufferTexture, m_bufferTexture[srcBufferIndex]);
#endif

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

	const float width = static_cast<float>(m_gameWidth > 0 ? m_gameWidth : SCREEN_WIDTH);
	const float height = static_cast<float>(m_gameHeight > 0 ? m_gameHeight : SCREEN_HEIGHT);
	XMMATRIX projection;
	projection = XMMatrixOrthographicOffCenterLH(0.0f, width, height, 0.0f, 0.0f, 1.0f);
	SetProjectionMatrix(projection);
}


void Renderer::SetWorldMatrix(XMMATRIX WorldMatrix)
{
	XMStoreFloat4x4(&m_PerObject.World, XMMatrixTranspose(WorldMatrix));
	m_DeviceContext->UpdateSubresource(m_PerObjectBuffer, 0, nullptr, &m_PerObject, 0, 0);
}

void Renderer::SetReceiveShadow(bool receiveShadow)
{
    m_PerObject.ReceiveShadow = receiveShadow ? 1u : 0u;
    m_DeviceContext->UpdateSubresource(m_PerObjectBuffer, 0, nullptr, &m_PerObject, 0, 0);
}

void Renderer::SetViewMatrix(XMMATRIX ViewMatrix)
{
    m_ViewMatrix = ViewMatrix;
	XMStoreFloat4x4(&m_PerFrame.View, XMMatrixTranspose(ViewMatrix));
	m_DeviceContext->UpdateSubresource(m_PerFrameBuffer, 0, nullptr, &m_PerFrame, 0, 0);
}

void Renderer::SetCameraPosition(const Vector3& position)
{
    // PBRの視線方向をワールド座標で計算できるよう、カメラ移動時に更新する。
    m_PerFrame.CameraPosition = XMFLOAT3(position.x, position.y, position.z);
    m_DeviceContext->UpdateSubresource(m_PerFrameBuffer, 0, nullptr, &m_PerFrame, 0, 0);
}

void Renderer::SetProjectionMatrix(XMMATRIX ProjectionMatrix)
{
    m_ProjectionMatrix = ProjectionMatrix;
	XMStoreFloat4x4(&m_PerFrame.Projection, XMMatrixTranspose(ProjectionMatrix));
	m_DeviceContext->UpdateSubresource(m_PerFrameBuffer, 0, nullptr, &m_PerFrame, 0, 0);

}

void Renderer::SetLight( LIGHT Light )
{
	m_Light = Light;
	m_DeviceContext->UpdateSubresource(m_LightingBuffer, 0, nullptr, &Light, 0, 0);
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
    if (m_CurrentRenderPhase == RenderPhase::Shadow)
    {
        m_DeviceContext->RSSetState(m_ShadowRasterizerState);
        m_CurrentCullMode = cull;
        return;
    }

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

void Renderer::SubmitDrawItem(RenderPhase phase, DrawItem item)
{
    if (IsPostProcessPhase(phase) || phase >= RenderPhase::Count)
        return;

    item.submitOrder = m_SubmitOrder++;
    m_DrawQueues[static_cast<size_t>(phase)].push_back(item);
}

void Renderer::SubmitDrawCommand(RenderPhase phase, DrawItem item)
{
    SubmitDrawItem(phase, item);
}
