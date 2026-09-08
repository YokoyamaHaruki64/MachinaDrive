#pragma once

#include "System/Core/main.h"
#include "Graphics/GraphicInc.h"

#include <array>
#include <string_view>



struct VERTEX_3D
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Diffuse;
	XMFLOAT2 TexCoord;
};



struct LIGHT
{
	BOOL		Enable;
	BOOL		Dummy[3];
	XMFLOAT4	Direction;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Ambient;
};


// common.hlsl の PerFrame（b0）と同じメモリ配置を表す。
struct PerFrame
{
	XMFLOAT4X4	View;
	XMFLOAT4X4	Projection;
	XMFLOAT3	CameraPosition;
	float		Time;
};


// common.hlsl の PerObject（b1）と同じメモリ配置を表す。
struct PerObject
{
	XMFLOAT4X4	World;
};


class Material;
class GraphicsShaderPass;
class PostProcess;

// 1回の描画を表す、フレーム内限定の提出データ。
struct DrawItem
{
    uint64_t sortKey{};
    class RendererBase* renderer{};
    uint32_t meshID{};
    uint64_t submitOrder{};
};

class Renderer
{
private:

	static D3D_FEATURE_LEVEL       m_FeatureLevel;

	static ID3D11Device*           m_Device;
	static ID3D11DeviceContext*    m_DeviceContext;
	static IDXGISwapChain*         m_SwapChain;
    static ID3D11Texture2D*        m_BackBufferTexture;
    static ID3D11RenderTargetView* m_BackBufferRTV;
	static ID3D11DepthStencilView* m_DepthStencilView;

    // 画面描画用のPing-PongバッファはRendererが所有する。
    static ID3D11Texture2D* m_bufferTexture[2];
    static ID3D11RenderTargetView* m_bufferRTV[2];
    static ID3D11ShaderResourceView* m_bufferSRV[2];
    static UINT m_gameWidth;
    static UINT m_gameHeight;
    static int srcBufferIndex;
    static int dstBufferIndex;

	static ID3D11Buffer*			m_PerFrameBuffer;
	static ID3D11Buffer*			m_PerObjectBuffer;
	static ID3D11Buffer*			m_LightingBuffer;
    static std::array<ID3D11Buffer*, static_cast<size_t>(VertexStreamType::Count)> m_FallbackVertexBuffers;

    static std::array<ID3D11DepthStencilState*, 14> m_DepthStates;

    static ID3D11RasterizerState* m_RasterizerState;
    static ID3D11RasterizerState* m_RasterizerStateNoCull;
    static ID3D11RasterizerState* m_RasterizerStateFront;

	static ID3D11BlendState*		m_BlendState;
    static ID3D11BlendState*        m_BlendStateAdd;
	static ID3D11BlendState*		m_BlendStateATC;
	static ID3D11SamplerState*		m_LinearSampler;
	static ID3D11SamplerState*		m_ClampSampler;
	static ID3D11SamplerState*		m_ShadowSampler;

    static XMMATRIX m_ViewMatrix;
    static XMMATRIX m_ProjectionMatrix;
    static PerFrame m_PerFrame;
    static PerObject m_PerObject;

    static std::array<std::vector<DrawItem>, static_cast<size_t>(RenderPhase::Count)> m_DrawQueues;
    static std::vector<PostProcess*> m_postEffects;
    static uint64_t m_SubmitOrder;

    static BlendMode m_CurrentBlendMode;
    static CullMode m_CurrentCullMode;

    static void CreateDepthStencilStates();
    static void CreateFallbackVertexBuffers();
    static bool CreateGameRenderTargets(UINT width, UINT height);
    static void ReleaseGameRenderTargets();
    static void SetSrcRTV();
    static ID3D11DepthStencilState* GetDepthStencilState(DepthTestMode testMode, DepthWriteMode writeMode);

public:
	static void Init();
	static void Uninit();
	static void Begin();
    static void Draw();
	static void End();
    static bool ResizeBackBuffer(UINT width, UINT height);
	static bool ResizeGameRenderTarget(UINT width, UINT height);
    static void BeginEditorFrame();

	static void SetDepthEnable(bool Enable);
	static void SetATCEnable(bool Enable);
	static void SetWorldViewProjection2D();
	static void SetWorldMatrix(XMMATRIX WorldMatrix);
	static void SetViewMatrix(XMMATRIX ViewMatrix);
	static void SetProjectionMatrix(XMMATRIX ProjectionMatrix);
	static void SetLight(LIGHT Light);
    static void SetBlendMode(BlendMode mode);
    static void SetRasterizerState(CullMode cull);
    static void BindRenderState(const RenderState& renderState);

	static ID3D11Device* GetDevice( void ){ return m_Device; }
	static ID3D11DeviceContext* GetDeviceContext( void ){ return m_DeviceContext; }
    static ID3D11Buffer* GetFallbackVertexBuffer(VertexStreamType streamType);

    static XMMATRIX GetViewMatrix();
    static XMMATRIX GetProjectionMatrix();
    static BlendMode GetCurrentBlendMode() { return m_CurrentBlendMode; }
    static CullMode GetCurrentCullMode() { return m_CurrentCullMode; }

    static void SubmitDrawItem(RenderPhase phase, DrawItem item);
    static void SubmitDrawCommand(RenderPhase phase, DrawItem item);
    static bool RegisterPostEffect(PostProcess* postEffect);
    static void UnregisterPostEffect(PostProcess* postEffect);
    static PostProcess* FindPostEffect(std::string_view name);
    static bool IsPostProcessMaterialUsed(const Material* material, const PostProcess* excludedPostEffect = nullptr);
    static void ClearPostEffects();
    static void ResizePostEffects();

    static bool SetRenderTarget(TextureHandle renderTexture, bool useDepthStencil = false);
    static void SetDstRTV();
    static ID3D11ShaderResourceView* GetSrcSRV() { return m_bufferSRV[srcBufferIndex]; }
    static UINT GetGameWidth() { return m_gameWidth; }
    static UINT GetGameHeight() { return m_gameHeight; }

    static void SwapBuffers() { std::swap(srcBufferIndex, dstBufferIndex); }
};
