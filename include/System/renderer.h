#pragma once

#include "System/main.h"
#include "Graphics/GraphicInc.h"

#include <array>



struct VERTEX_3D
{
	XMFLOAT3 Position;
	XMFLOAT3 Normal;
	XMFLOAT4 Diffuse;
	XMFLOAT2 TexCoord;
};



struct MATERIAL
{
	XMFLOAT4	Ambient;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Specular;
	XMFLOAT4	Emission;
	float		Shininess;
	BOOL		TextureEnable;
	float		Dummy[2];
};



struct LIGHT
{
	BOOL		Enable;
	BOOL		Dummy[3];
	XMFLOAT4	Direction;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Ambient;
};


class Material;
class GraphicsShaderPass;

// 1回の描画を表す、フレーム内限定の提出データ。
// 現在は RendererBase 互換経路を使用し、Material 描画経路を段階的に追加する。
struct DrawItem
{
    uint64_t sortKey{};
    class RendererBase* renderer{};
    const Material* material{};
    const GraphicsShaderPass* shaderPass{};
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
	static ID3D11DepthStencilView* m_DepthStencilView;

    // バッファ用のテクスチャとビュー
    static ID3D11Texture2D* m_bufferTexture[2];
    static ID3D11RenderTargetView* m_bufferRTV[2];
    static ID3D11ShaderResourceView* m_bufferSRV[2];
    static int srcBufferIndex;
    static int dstBufferIndex;

	static ID3D11Buffer*			m_WorldBuffer;
	static ID3D11Buffer*			m_ViewBuffer;
	static ID3D11Buffer*			m_ProjectionBuffer;
	static ID3D11Buffer*			m_MaterialBuffer;
	static ID3D11Buffer*			m_LightBuffer;
    static ID3D11Buffer*            m_UVBuffer;

	static ID3D11DepthStencilState* m_DepthStateEnable;
	static ID3D11DepthStencilState* m_DepthStateDisable;

    static ID3D11RasterizerState* m_RasterizerState;
    static ID3D11RasterizerState* m_RasterizerStateNoCull;

	static ID3D11BlendState*		m_BlendState;
    static ID3D11BlendState*        m_BlendStateAdd;
	static ID3D11BlendState*		m_BlendStateATC;

    static XMMATRIX m_ViewMatrix;
    static XMMATRIX m_ProjectionMatrix;

    static std::array<std::vector<DrawItem>, static_cast<size_t>(RenderPhase::Count)> m_DrawQueues;
    static uint64_t m_SubmitOrder;

    static BlendMode m_CurrentBlendMode;
    static CullMode m_CurrentCullMode;

public:
	static void Init();
	static void Uninit();
	static void Begin();
    static void Draw();
	static void End();

	static void SetDepthEnable(bool Enable);
	static void SetATCEnable(bool Enable);
	static void SetWorldViewProjection2D();
	static void SetWorldMatrix(XMMATRIX WorldMatrix);
	static void SetViewMatrix(XMMATRIX ViewMatrix);
	static void SetProjectionMatrix(XMMATRIX ProjectionMatrix);
	static void SetMaterial(MATERIAL Material);
	static void SetLight(LIGHT Light);
    static void SetUV(XMFLOAT4 UV);
    static void SetBlendMode(BlendMode mode);
    static void SetRasterizerState(CullMode cull);

	static ID3D11Device* GetDevice( void ){ return m_Device; }
	static ID3D11DeviceContext* GetDeviceContext( void ){ return m_DeviceContext; }

    static XMMATRIX GetViewMatrix();
    static XMMATRIX GetProjectionMatrix();
    static BlendMode GetCurrentBlendMode() { return m_CurrentBlendMode; }
    static CullMode GetCurrentCullMode() { return m_CurrentCullMode; }

	static void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName);
	static void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);

    static void SubmitDrawItem(RenderPhase phase, DrawItem item);
    static void SubmitDrawCommand(RenderPhase phase, DrawItem item);

    static void SetDstRTV()
    {
        m_DeviceContext->OMSetRenderTargets(1, &m_bufferRTV[dstBufferIndex], m_DepthStencilView);
    }
    static ID3D11ShaderResourceView* GetSrcSRV() { return m_bufferSRV[srcBufferIndex]; }

    static void SwapBuffers() { std::swap(srcBufferIndex, dstBufferIndex); }
};
