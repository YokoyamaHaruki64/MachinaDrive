/*=============================================================================

File   : [GraphicInc.h]
Desc   : 描画機能を利用する側が共有する値型・GPU形状型・描画状態を定義する

------------------------------------------------------------------------------

Date   : 2026/04/19
Author : Yokoyama Haruki

===============================================================================*/

#ifndef _GRAPHIC_INC_H_
#define _GRAPHIC_INC_H_

#include <variant>
#include <array>
#include <bitset>
#include <d3d11.h>
#include <utility>
#include <vector>
#include <wrl/client.h>
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix3x3.h"
#include "math/Matrix4x4.h"
#include "Common/Color.h"

using ShaderPropertyID = UINT;
using ShaderID = UINT;
using MaterialID = UINT;
using MaterialPropertyID = uint64_t;
using TextureID = int;

class Material;
class GraphicsShader;
class GraphicsShaderPass;

static constexpr UINT INVALID_PROPERTY_OFFSET = UINT_MAX;

constexpr MaterialPropertyID MakeMaterialPropertyID(MaterialID materialID, ShaderPropertyID shaderPropertyID)
{
    return (static_cast<MaterialPropertyID>(materialID) << 32) | shaderPropertyID;
}

constexpr MaterialID GetMaterialID(MaterialPropertyID materialPropertyID)
{
    return static_cast<MaterialID>(materialPropertyID >> 32);
}

constexpr ShaderPropertyID GetShaderPropertyID(MaterialPropertyID materialPropertyID)
{
    return static_cast<ShaderPropertyID>(materialPropertyID);
}

struct TextureHandle
{
    TextureID id{ -1 }; // テクスチャID、-1は無効なテクスチャを表す
    TextureHandle() = default;
    explicit TextureHandle(TextureID id) : id(id) {}
    bool IsValid() const { return id >= 0; }
};

enum class RenderPhase : uint8_t
{
    Shadow,
    Opaque,
    Transparent,
    PostProcess,
    Overlay,

    Count,
};

enum class BlendMode : uint8_t
{
    None = 0,
    AlphaBlend = 1,
    Add = 2,
};
enum class CullMode : uint8_t
{
    None = 0,
    Back = 1,
    Front = 2,
};
enum class DepthTestMode : uint8_t
{
    None = 0,
    Less = 1,
    LessEqual = 2,
    Greater = 3,
    GreaterEqual = 4,
    Equal = 5,
    NotEqual = 6,

    Count,
};
enum class DepthWriteMode : uint8_t
{
    None = 0,
    Write = 1,

    Count,
};

struct RenderState
{
    RenderPhase renderPhase{ RenderPhase::Opaque };

    BlendMode blendMode{ BlendMode::None };
    CullMode cullMode{ CullMode::Back };
    DepthTestMode depthTestMode{ DepthTestMode::LessEqual };
    DepthWriteMode depthWriteMode{ DepthWriteMode::Write };

    uint32_t GetModeBitMask() const
    {
        uint32_t mask = 0;
        mask |= static_cast<uint32_t>(blendMode);
        mask |= static_cast<uint32_t>(cullMode) << 4;
        mask |= static_cast<uint32_t>(depthTestMode) << 8;
        mask |= static_cast<uint32_t>(depthWriteMode) << 12;
        return mask;
    }
};


using PropertyValue = std::variant<
    float,
    int,
    bool,
    Vector2,
    Vector3,
    Vector4,
    Matrix3x3,
    Matrix4x4,
    Color,
    TextureHandle>;

enum class VertexStreamType
{
    Position,
    NormalTangent,
    PrimaryTexCoord,
    SecondaryTexCoord,
    ExtraTexCoord,
    Color,
    Count,

    Error = -1
};

struct PositionVertex
{
    Vector3 position;

    // Format : DXGI_FORMAT_R32G32B32_FLOAT
    // Offset : 0
    // Stride : 12bytes
    // Slot : 0
};

struct NormalTangentVertex
{
    Vector3 normal;
    Vector4 tangent;

    // NORMAL0
    // Format : DXGI_FORMAT_R32G32B32_FLOAT
    // Offset : 0

    // TANGENT0
    // Format : DXGI_FORMAT_R32G32B32A32_FLOAT
    // Offset : 12

    // Stride : 28bytes
    // Slot : 1
};

struct PrimaryTexCoordVertex
{
    Vector4 texCoord;

    // Format : DXGI_FORMAT_R32G32B32A32_FLOAT
    // Offset : 0
    // Stride : 16bytes
    // Slot : 2
};

struct SecondaryTexCoordVertex
{
    Vector4 texCoord;

    // Format : DXGI_FORMAT_R32G32B32A32_FLOAT
    // Offset : 0
    // Stride : 16bytes
    // Slot : 3
};

struct ExtraTexCoordVertex
{
    Vector4 texCoord2;
    Vector4 texCoord3;
    Vector4 texCoord4;
    Vector4 texCoord5;
    Vector4 texCoord6;
    Vector4 texCoord7;

    // Format : DXGI_FORMAT_R32G32B32A32_FLOAT
    // Offset : 0 + 16 * N - 2 ( N = 2 ~ 7 )
    // Stride : 96bytes
    // Slot : 4
};

struct ColorVertex
{
    Color color;
    // Format : DXGI_FORMAT_R32G32B32A32_FLOAT
    // Offset : 0
    // Stride : 16bytes
    // Slot : 5
};

// GPU上の共有可能な形状データ。頂点属性はStreamTypeごとの独立した頂点バッファで保持する。
struct Mesh
{
private:
    static constexpr size_t VERTEX_STREAM_COUNT = static_cast<size_t>(VertexStreamType::Count);

    std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, VERTEX_STREAM_COUNT> m_vertexBuffers{};
    std::bitset<VERTEX_STREAM_COUNT> m_vertexStreamMask{};
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer{};

public:
    uint32_t materialSlot{};
    unsigned int indexCount{};
    uint32_t meshID{};

    void SetVertexBuffer(VertexStreamType streamType, Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer)
    {
        const size_t index = static_cast<size_t>(streamType);
        if (streamType == VertexStreamType::Error || index >= VERTEX_STREAM_COUNT)
            return;

        m_vertexBuffers[index] = std::move(vertexBuffer);
        m_vertexStreamMask.set(index, m_vertexBuffers[index] != nullptr);
    }

    bool HasVertexStream(VertexStreamType streamType) const
    {
        if (streamType == VertexStreamType::Error || static_cast<size_t>(streamType) >= VERTEX_STREAM_COUNT)
            return false;

        return m_vertexStreamMask.test(static_cast<size_t>(streamType));
    }

    ID3D11Buffer* GetVertexBuffer(VertexStreamType streamType) const
    {
        const size_t index = static_cast<size_t>(streamType);
        return HasVertexStream(streamType) ? m_vertexBuffers[index].Get() : nullptr;
    }

    static UINT GetVertexStreamStride(VertexStreamType streamType)
    {
        switch (streamType)
        {
        case VertexStreamType::Position: return sizeof(PositionVertex);
        case VertexStreamType::NormalTangent: return sizeof(NormalTangentVertex);
        case VertexStreamType::PrimaryTexCoord: return sizeof(PrimaryTexCoordVertex);
        case VertexStreamType::SecondaryTexCoord: return sizeof(SecondaryTexCoordVertex);
        case VertexStreamType::ExtraTexCoord: return sizeof(ExtraTexCoordVertex);
        case VertexStreamType::Color: return sizeof(ColorVertex);
        default: return 0;
        }
    }

    std::vector<VertexStreamType> GetVertexStreamTypes() const
    {
        std::vector<VertexStreamType> streamTypes;
        for (size_t index = 0; index < VERTEX_STREAM_COUNT; ++index)
        {
            if (m_vertexStreamMask.test(index))
                streamTypes.push_back(static_cast<VertexStreamType>(index));
        }
        return streamTypes;
    }

    void SetIndexBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer) { m_indexBuffer = std::move(indexBuffer); }
    ID3D11Buffer* GetIndexBuffer() const { return m_indexBuffer.Get(); }
    void Clear()
    {
        m_vertexBuffers = {};
        m_vertexStreamMask.reset();
        m_indexBuffer.Reset();
        materialSlot = 0;
        indexCount = 0;
        meshID = 0;
    }
};

struct PerFrameData
{
    Matrix4x4 viewMatrix;
    Matrix4x4 projectionMatrix;
    Vector3 cameraPosition;
    float time; // ゲーム時間やフレーム時間など、シェーダーで使用する時間情報
};

struct PerObjectData
{
    Matrix4x4 worldMatrix;
    Color objectColor;
};

struct LightData
{
    Vector3 lightDirection;
    float padding1; // アライメントのためのパディング
    Color lightColor;
    float intensity;
    float padding2[3]; // アライメントのためのパディング
};

static constexpr int RESERVE_TEXTURE_SLOT_COUNT = 16;
static constexpr int MAX_TEXTURE_SLOT_COUNT = 32;

static constexpr const char* DEFAULT_SHADER_MODEL_VS = "vs_5_0";
static constexpr const char* DEFAULT_SHADER_MODEL_PS = "ps_5_0";

static constexpr const char* PER_FRAME_CBUFFER_NAME = "PerFrame";
static constexpr const char* PER_OBJECT_CBUFFER_NAME = "PerObject";
static constexpr const char* LIGHT_CBUFFER_NAME = "Light";

#endif // _GRAPHIC_INC_H_
