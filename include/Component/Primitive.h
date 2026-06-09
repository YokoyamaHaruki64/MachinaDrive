#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "math/Vector3.h"
#include "System/GameObject.h"
#include "System/RendererBase.h"
using Microsoft::WRL::ComPtr;


class PrimitiveRenderer : public RendererBase
{
protected:
    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;

    ComPtr<ID3D11VertexShader> m_vertexShader;
    ComPtr<ID3D11PixelShader> m_pixelShader;
    ComPtr<ID3D11InputLayout> m_inputLayout;

    ComPtr<ID3D11ShaderResourceView> m_texture;
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    int m_indexCount = 0;

    void LoadTexture(const wchar_t* textureFile);
    void CreateVertexShaderAndInputLayout(const char* vertexShaderFile);
    void CreatePixelShader(const char* pixelShaderFile);
    void CreateVertexBuffer(const void* vertexData, size_t vertexDataSize);
    void CreateIndexBuffer(const unsigned int* indexData, size_t indexDataSize);

    static const char* DEFAULT_VERTEX_SHADER_FILE;
    static const char* DEFAULT_PIXEL_SHADER_FILE;
public:
    PrimitiveRenderer() = delete;
    PrimitiveRenderer(GameObject* owner) : RendererBase(owner) {}
    virtual ~PrimitiveRenderer() {}
    void Draw() override;
};

class Quad : public PrimitiveRenderer
{
private:
    static constexpr int QUAD_INDEX_COUNT = 6;
    static constexpr unsigned int QUAD_INDEX_BUFFER[] = { 0, 1, 2, 2, 1, 3 };
public:
    Quad() = delete;
    Quad(GameObject* owner,
        const wchar_t* textureFile,
        const char* vertexShaderFile = DEFAULT_VERTEX_SHADER_FILE,
        const char* pixelShaderFile = DEFAULT_PIXEL_SHADER_FILE);
};

class Plane : public PrimitiveRenderer
{
private:
    static constexpr int PLANE_INDEX_COUNT = 6;
    static constexpr unsigned int PLANE_INDEX_BUFFER[] = { 0, 1, 2, 2, 1, 3 };
public:
    Plane() = delete;
    Plane(GameObject* owner,
        const wchar_t* textureFile,
        const char* vertexShaderFile = DEFAULT_VERTEX_SHADER_FILE,
        const char* pixelShaderFile = DEFAULT_PIXEL_SHADER_FILE);
    
};

class Cube : public PrimitiveRenderer
{
private:
    static constexpr int CUBE_INDEX_COUNT = 36;
    // 24頂点のキューブのインデックスデータ
    static constexpr unsigned int CUBE_INDEX_BUFFER[] =
    {
        // 天
        0,1,2,		1,3,2,
        // 地
        4,5,6,		5,7,6,
        // 背面
        8,9,10,		9,11,10,
        // 正面
        12,13,14,	13,15,14,
        // 右面
        16,17,18,	17,19,18,
        // 左面
        20,21,22,	21,23,22
    };

public:
    Cube() = delete;
    Cube(GameObject* owner,
        const wchar_t* textureFile,
        const char* vertexShaderFile = DEFAULT_VERTEX_SHADER_FILE,
        const char* pixelShaderFile = DEFAULT_PIXEL_SHADER_FILE);


};
