#pragma once

#include "Graphics/ModelAsset.h"
#include "Graphics/Material.h"
#include "math/Vector3.h"
#include "System/GameObject.h"
#include "System/RendererBase.h"

#include <vector>

class GraphicsShader;


class PrimitiveRenderer : public RendererBase
{
protected:
    Mesh m_mesh{};
    Material* m_materialInstance{};
    TextureHandle m_texture{};
    int m_textureWidth = 0;
    int m_textureHeight = 0;

    float m_uvOffsetX = 0.0f;
    float m_uvOffsetY = 0.0f;
    float m_uvWidth = 1.0f;
    float m_uvHeight = 1.0f;

    void CreateMaterial(const char* shaderFilePath);
    void LoadTexture(const wchar_t* textureFile);
    void CreateMesh(
        const std::vector<PositionVertex>& positions,
        const std::vector<NormalTangentVertex>& normalTangents,
        const std::vector<PrimaryTexCoordVertex>& texCoords,
        const std::vector<ColorVertex>& colors,
        const unsigned int* indexData,
        size_t indexCount);
    void BindMesh(const XMMATRIX& worldMatrix, const Color& color) const;

public:
    PrimitiveRenderer() = delete;
    PrimitiveRenderer(GameObject* owner) : RendererBase(owner) {}
    virtual ~PrimitiveRenderer() {}
    void Draw() override;

    void SetUVRect(float offsetX, float offsetY, float width, float height)
    {
        m_uvOffsetX = offsetX;
        m_uvOffsetY = offsetY;
        m_uvWidth = width;
        m_uvHeight = height;
    }

    void SetUVOffset(float offsetX, float offsetY)
    {
        m_uvOffsetX = offsetX;
        m_uvOffsetY = offsetY;
    }

    void SetUVSize(float width, float height)
    {
        m_uvWidth = width;
        m_uvHeight = height;
    }
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
        const char* vertexShaderFile = nullptr,
        const char* pixelShaderFile = nullptr);
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
        const char* vertexShaderFile = nullptr,
        const char* pixelShaderFile = nullptr);
    
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
        const char* vertexShaderFile = nullptr,
        const char* pixelShaderFile = nullptr);


};
