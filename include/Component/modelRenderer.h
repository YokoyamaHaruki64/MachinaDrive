#pragma once

#include "System/renderer.h"
#include "System/RendererBase.h"
#include <string>
#include <unordered_map>
#include <d3d11.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// マテリアル構造体
struct MODEL_MATERIAL
{
	char						Name[256];
	MATERIAL					Material;
	char						TextureName[256];
	TextureHandle					Texture;

};


// 描画サブセット。形状範囲とMaterial slotの対応だけを持つ。
struct SubMesh
{
	unsigned int	StartIndex;
	unsigned int	IndexNum;
	uint32_t		MaterialSlot;
};

// OBJロード中だけ使用する旧形式のサブセット。
struct SUBSET
{
    unsigned int StartIndex;
    unsigned int IndexNum;
    MODEL_MATERIAL Material;
};


// モデル構造体
struct MODEL_OBJ
{
	VERTEX_3D		*VertexArray;
	unsigned int	VertexNum;

	unsigned int	*IndexArray;
	unsigned int	IndexNum;

	SUBSET			*SubsetArray;
	unsigned int	SubsetNum;
};

// GPU上の共有可能な形状データ
struct Mesh
{
	ID3D11Buffer*	VertexBuffer;
	ID3D11Buffer*	IndexBuffer;
	std::vector<SubMesh> SubMeshes;
	uint32_t MeshID{};
};

// 形状とMaterial slotを分離したモデルアセット
struct ModelAsset
{
	Mesh Mesh{};
	std::vector<MODEL_MATERIAL> MaterialSlots{};
};

class Transform;
class ModelRenderer : public RendererBase
{
public:
    enum InverseAxis
    {
        X = 1,
        Y = 1 << 1,
        Z = 1 << 2
    };
private:

	static std::unordered_map<std::string, ModelAsset*> m_ModelPool;

	static void LoadModel(const char *FileName, ModelAsset *Model);
	static void LoadObj( const char *FileName, MODEL_OBJ *ModelObj );
	static void LoadMaterial( const char *FileName, MODEL_MATERIAL **MaterialArray, unsigned int *MaterialNum );

	ModelAsset* m_Model{};
    ComPtr<ID3D11VertexShader> m_VertexShader{};
    ComPtr<ID3D11PixelShader> m_PixelShader{};
    ComPtr<ID3D11InputLayout> m_InputLayout{};

    uint8_t m_InverseAxisFlags = 0; // 反転させる軸をビットフラグで管理
public:

	static void Preload( const char *FileName );
	static void UnloadAll();

    ModelRenderer() = delete;
    ModelRenderer(GameObject* Object,
        const char* modelFileName,
        const char* vertexShaderFileName,
        const char* pixelShaderFileName,
        uint8_t inverseAxisFlags = 0) : RendererBase(Object), m_InverseAxisFlags(inverseAxisFlags)
    {
        Load(modelFileName, vertexShaderFileName, pixelShaderFileName);
    }

	void Load(
        const char* modelFileName,
        const char* vertexShaderFileName,
        const char* pixelShaderFileName);

	void Draw() override;

};
