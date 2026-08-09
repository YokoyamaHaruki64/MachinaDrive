#include "pch.h"

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include "Component/modelRenderer.h"

#include "System/renderer.h"
#include "Graphics/TextureManager.h"
#include "System/GameObject.h"
#include "Component/Transform.h"

#include "math/MathCommon.h"



std::unordered_map<std::string, ModelAsset*> ModelRenderer::m_ModelPool;


void ModelRenderer::Load(const char* modelFileName, const char* vertexShaderFileName, const char* pixelShaderFileName)
{
    // Renderer単位のシェーダーを先に作成し、形状データはプールから共有する。
    Renderer::CreateVertexShader(m_VertexShader.GetAddressOf(), m_InputLayout.GetAddressOf(), vertexShaderFileName);
    Renderer::CreatePixelShader(m_PixelShader.GetAddressOf(), pixelShaderFileName);

    // 同一モデルはGPUバッファとMaterial slotを再利用する。
    if (m_ModelPool.find(modelFileName) != m_ModelPool.end())
    {
        m_Model = m_ModelPool[modelFileName];
        return;
    }

    m_Model = new ModelAsset;
    LoadModel(modelFileName, m_Model);

    m_ModelPool[modelFileName] = m_Model;

}

void ModelRenderer::Draw()
{
    Transform* t = transform();

	// 互換経路では、従来どおりinterleaved頂点バッファをslot 0へ設定する。
	UINT stride = sizeof(VERTEX_3D);
	UINT offset = 0;
	Renderer::GetDeviceContext()->IASetVertexBuffers(0, 1, &m_Model->Mesh.VertexBuffer, &stride, &offset);

	// 形状全体で共有するインデックスバッファとトポロジを設定する。
	Renderer::GetDeviceContext()->IASetIndexBuffer(m_Model->Mesh.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// プリミティブトポロジ設定
	Renderer::GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // シェーダー設定
    Renderer::GetDeviceContext()->VSSetShader(m_VertexShader.Get(), nullptr, 0);
    Renderer::GetDeviceContext()->PSSetShader(m_PixelShader.Get(), nullptr, 0);

    // 入力レイアウト設定
    Renderer::GetDeviceContext()->IASetInputLayout(m_InputLayout.Get());

    // 定数バッファ設定
    if (t)
    {
        XMMATRIX axisInverse = XMMatrixRotationRollPitchYaw(
            m_InverseAxisFlags & InverseAxis::X ? Math::PI : 0.0f,
            m_InverseAxisFlags & InverseAxis::Y ? Math::PI : 0.0f,
            m_InverseAxisFlags & InverseAxis::Z ? Math::PI : 0.0f);
        XMMATRIX world = axisInverse * CalcWorldMatrix();

        Renderer::SetWorldMatrix(world);
    }

    // UV設定
    Renderer::SetUV(XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f));

	// SubMeshは描画範囲だけを持ち、実際のMaterialはslotから解決する。
	for (const SubMesh& subMesh : m_Model->Mesh.SubMeshes)
	{
		const MODEL_MATERIAL& material = m_Model->MaterialSlots[subMesh.MaterialSlot];

		// 旧Material定数バッファとテクスチャを互換経路へ設定する。
		Renderer::SetMaterial(material.Material);

		if(material.Texture.IsValid())
		{
			ID3D11ShaderResourceView* texture = TextureManager::GetShaderResourceView(material.Texture);
			Renderer::GetDeviceContext()->PSSetShaderResources(0, 1, &texture);
		}

		// SubMeshのindex範囲だけを描画する。
		Renderer::GetDeviceContext()->DrawIndexed(subMesh.IndexNum, subMesh.StartIndex, 0);
	}

}

void ModelRenderer::Preload(const char *FileName)
{
	if (m_ModelPool.count(FileName) > 0)
	{
		return;
	}

	ModelAsset* model = new ModelAsset;
	LoadModel(FileName, model);

	m_ModelPool[FileName] = model;

}


void ModelRenderer::UnloadAll()
{
	// ModelAssetが所有するGPUリソースとMaterial slotのテクスチャを解放する。
	for (std::pair<const std::string, ModelAsset*> pair : m_ModelPool)
	{
		pair.second->Mesh.VertexBuffer->Release();
		pair.second->Mesh.IndexBuffer->Release();

		delete pair.second;
	}

	m_ModelPool.clear();
}

void ModelRenderer::LoadModel( const char *FileName, ModelAsset *Model)
{
	MODEL_OBJ modelObj;
	LoadObj( FileName, &modelObj );

	// CPUで読み込んだOBJ頂点を、共有MeshのGPU頂点バッファへ転送する。
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof( VERTEX_3D ) * modelObj.VertexNum;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory( &sd, sizeof(sd) );
		sd.pSysMem = modelObj.VertexArray;

		Renderer::GetDevice()->CreateBuffer( &bd, &sd, &Model->Mesh.VertexBuffer );
	}


	// すべてのSubMeshで共有するインデックスバッファを生成する。
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof(bd) );
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof( unsigned int ) * modelObj.IndexNum;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA sd;
		ZeroMemory( &sd, sizeof(sd) );
		sd.pSysMem = modelObj.IndexArray;

		Renderer::GetDevice()->CreateBuffer( &bd, &sd, &Model->Mesh.IndexBuffer );
	}

	// OBJの各サブセットを、描画範囲とMaterial slotへ分離して保存する。
	{
		Model->Mesh.SubMeshes.resize(modelObj.SubsetNum);
		Model->MaterialSlots.resize(modelObj.SubsetNum);

		for( unsigned int i = 0; i < modelObj.SubsetNum; i++ )
		{
			Model->Mesh.SubMeshes[i] = { modelObj.SubsetArray[i].StartIndex, modelObj.SubsetArray[i].IndexNum, i };

			Model->MaterialSlots[i] = modelObj.SubsetArray[i].Material;
			const char* texturePath = modelObj.SubsetArray[i].Material.TextureName;
			if (texturePath[0] != '\0')
			{
				Model->MaterialSlots[i].Texture = TextureManager::LoadTexture(std::string(texturePath));
				Model->MaterialSlots[i].Material.TextureEnable = true;
			}
			else
			{
				Model->MaterialSlots[i].Material.TextureEnable = false;
			}

		}
	}

	delete[] modelObj.VertexArray;
	delete[] modelObj.IndexArray;
	delete[] modelObj.SubsetArray;

}






//モデル読込////////////////////////////////////////////
void ModelRenderer::LoadObj( const char *FileName, MODEL_OBJ *ModelObj )
{

	char dir[MAX_PATH];
	strcpy (dir, FileName );
	PathRemoveFileSpec(dir);





	XMFLOAT3	*positionArray;
	XMFLOAT3	*normalArray;
	XMFLOAT2	*texcoordArray;

	unsigned int	positionNum = 0;
	unsigned int	normalNum = 0;
	unsigned int	texcoordNum = 0;
	unsigned int	vertexNum = 0;
	unsigned int	indexNum = 0;
	unsigned int	in = 0;
	unsigned int	subsetNum = 0;

	MODEL_MATERIAL	*materialArray = nullptr;
	unsigned int	materialNum = 0;

	char str[256];
	char *s;
	char c;


	FILE *file;
	file = fopen( FileName, "rt" );
	assert(file);



	//要素数カウント
	while( true )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;

		if( strcmp( str, "v" ) == 0 )
		{
			positionNum++;
		}
		else if( strcmp( str, "vn" ) == 0 )
		{
			normalNum++;
		}
		else if( strcmp( str, "vt" ) == 0 )
		{
			texcoordNum++;
		}
		else if( strcmp( str, "usemtl" ) == 0 )
		{
			subsetNum++;
		}
		else if( strcmp( str, "f" ) == 0 )
		{
			in = 0;

			do
			{
				fscanf( file, "%s", str );
				vertexNum++;
				in++;
				c = fgetc( file );
			}
			while( c != '\n' && c!= '\r' );

			//四角は三角に分割
			if( in == 4 )
				in = 6;

			indexNum += in;
		}
	}


	//メモリ確保
	positionArray = new XMFLOAT3[ positionNum ];
	normalArray = new XMFLOAT3[ normalNum ];
	texcoordArray = new XMFLOAT2[ texcoordNum ];


	ModelObj->VertexArray = new VERTEX_3D[ vertexNum ];
	ModelObj->VertexNum = vertexNum;

	ModelObj->IndexArray = new unsigned int[ indexNum ];
	ModelObj->IndexNum = indexNum;

	ModelObj->SubsetArray = new SUBSET[ subsetNum ];
	ModelObj->SubsetNum = subsetNum;




	//要素読込
	XMFLOAT3 *position = positionArray;
	XMFLOAT3 *normal = normalArray;
	XMFLOAT2 *texcoord = texcoordArray;

	unsigned int vc = 0;
	unsigned int ic = 0;
	unsigned int sc = 0;


	fseek( file, 0, SEEK_SET );

	while( true )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;

		if( strcmp( str, "mtllib" ) == 0 )
		{
			//マテリアルファイル
			fscanf( file, "%s", str );

			char path[256];
			strcpy( path, dir );
			strcat( path, "\\" );
			strcat( path, str );

			LoadMaterial( path, &materialArray, &materialNum );
		}
		else if( strcmp( str, "o" ) == 0 )
		{
			//オブジェクト名
			fscanf( file, "%s", str );
		}
		else if( strcmp( str, "v" ) == 0 )
		{
			//頂点座標
			fscanf( file, "%f", &position->x );
			fscanf( file, "%f", &position->y );
			fscanf( file, "%f", &position->z );
			position++;
		}
		else if( strcmp( str, "vn" ) == 0 )
		{
			//法線
			fscanf( file, "%f", &normal->x );
			fscanf( file, "%f", &normal->y );
			fscanf( file, "%f", &normal->z );
			normal++;
		}
		else if( strcmp( str, "vt" ) == 0 )
		{
			//テクスチャ座標
			fscanf( file, "%f", &texcoord->x );
			fscanf( file, "%f", &texcoord->y );
			texcoord->x = 1.0f - texcoord->x;
			texcoord->y = 1.0f - texcoord->y;
			texcoord++;
		}
		else if( strcmp( str, "usemtl" ) == 0 )
		{
			//マテリアル
			fscanf( file, "%s", str );

			if( sc != 0 )
				ModelObj->SubsetArray[ sc - 1 ].IndexNum = ic - ModelObj->SubsetArray[ sc - 1 ].StartIndex;

			ModelObj->SubsetArray[ sc ].StartIndex = ic;


			for( unsigned int i = 0; i < materialNum; i++ )
			{
				if( strcmp( str, materialArray[i].Name ) == 0 )
				{
					ModelObj->SubsetArray[ sc ].Material.Material = materialArray[i].Material;
					strcpy( ModelObj->SubsetArray[ sc ].Material.TextureName, materialArray[i].TextureName );
					strcpy( ModelObj->SubsetArray[ sc ].Material.Name, materialArray[i].Name );

					break;
				}
			}

			sc++;
			
		}
		else if( strcmp( str, "f" ) == 0 )
		{
			//面
			in = 0;

			do
			{
				fscanf( file, "%s", str );

				s = strtok( str, "/" );	
				ModelObj->VertexArray[vc].Position = positionArray[ atoi( s ) - 1 ];
				if( s[ strlen( s ) + 1 ] != '/' )
				{
					//テクスチャ座標が存在しない場合もある
					s = strtok( nullptr, "/" );
					ModelObj->VertexArray[vc].TexCoord = texcoordArray[ atoi( s ) - 1 ];
				}
				s = strtok( nullptr, "/" );	
				ModelObj->VertexArray[vc].Normal = normalArray[ atoi( s ) - 1 ];

				ModelObj->VertexArray[vc].Diffuse = XMFLOAT4( 1.0f, 1.0f, 1.0f, 1.0f );

				ModelObj->IndexArray[ic] = vc;
				ic++;
				vc++;

				in++;
				c = fgetc( file );
			}
			while( c != '\n' && c != '\r' );

			//四角は三角に分割
			if( in == 4 )
			{
				ModelObj->IndexArray[ic] = vc - 4;
				ic++;
				ModelObj->IndexArray[ic] = vc - 2;
				ic++;
			}
		}
	}


	if( sc != 0 )
		ModelObj->SubsetArray[ sc - 1 ].IndexNum = ic - ModelObj->SubsetArray[ sc - 1 ].StartIndex;


	fclose( file );


	delete[] positionArray;
	delete[] normalArray;
	delete[] texcoordArray;
	delete[] materialArray;
}




//マテリアル読み込み///////////////////////////////////////////////////////////////////
void ModelRenderer::LoadMaterial( const char *FileName, MODEL_MATERIAL **MaterialArray, unsigned int *MaterialNum )
{

	char dir[MAX_PATH];
	strcpy(dir, FileName);
	PathRemoveFileSpec(dir);



	char str[256];

	FILE *file;
	file = fopen( FileName, "rt" );
	assert(file);

	MODEL_MATERIAL *materialArray;
	unsigned int materialNum = 0;

	//要素数カウント
	while( true )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;


		if( strcmp( str, "newmtl" ) == 0 )
		{
			materialNum++;
		}
	}


	//メモリ確保
	materialArray = new MODEL_MATERIAL[ materialNum ];


	//要素読込
	int mc = -1;

	fseek( file, 0, SEEK_SET );

	while( true )
	{
		fscanf( file, "%s", str );

		if( feof( file ) != 0 )
			break;


		if( strcmp( str, "newmtl" ) == 0 )
		{
			//マテリアル名
			mc++;
			fscanf( file, "%s", materialArray[ mc ].Name );
			strcpy( materialArray[ mc ].TextureName, "" );

			materialArray[mc].Material.Emission.x = 0.0f;
			materialArray[mc].Material.Emission.y = 0.0f;
			materialArray[mc].Material.Emission.z = 0.0f;
			materialArray[mc].Material.Emission.w = 0.0f;
		}
		else if( strcmp( str, "Ka" ) == 0 )
		{
			//アンビエント
			fscanf( file, "%f", &materialArray[ mc ].Material.Ambient.x );
			fscanf( file, "%f", &materialArray[ mc ].Material.Ambient.y );
			fscanf( file, "%f", &materialArray[ mc ].Material.Ambient.z );
			materialArray[ mc ].Material.Ambient.w = 1.0f;
		}
		else if( strcmp( str, "Kd" ) == 0 )
		{
			//ディフューズ
			fscanf( file, "%f", &materialArray[ mc ].Material.Diffuse.x );
			fscanf( file, "%f", &materialArray[ mc ].Material.Diffuse.y );
			fscanf( file, "%f", &materialArray[ mc ].Material.Diffuse.z );
			materialArray[ mc ].Material.Diffuse.w = 1.0f;
		}
		else if( strcmp( str, "Ks" ) == 0 )
		{
			//スペキュラ
			fscanf( file, "%f", &materialArray[ mc ].Material.Specular.x );
			fscanf( file, "%f", &materialArray[ mc ].Material.Specular.y );
			fscanf( file, "%f", &materialArray[ mc ].Material.Specular.z );
			materialArray[ mc ].Material.Specular.w = 1.0f;
		}
		else if( strcmp( str, "Ns" ) == 0 )
		{
			//スペキュラ強度
			fscanf( file, "%f", &materialArray[ mc ].Material.Shininess );
		}
		else if( strcmp( str, "d" ) == 0 )
		{
			//アルファ
			fscanf( file, "%f", &materialArray[ mc ].Material.Diffuse.w );
		}
		else if( strcmp( str, "map_Kd" ) == 0 )
		{
			//テクスチャ
			fscanf( file, "%s", str );

			char path[256];
			strcpy( path, dir );
			strcat( path, "\\" );
			strcat( path, str );

			strcat( materialArray[ mc ].TextureName, path );
		}
	}

	fclose(file);

	*MaterialArray = materialArray;
	*MaterialNum = materialNum;
}

