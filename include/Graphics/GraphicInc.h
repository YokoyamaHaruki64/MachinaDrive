/*=============================================================================

File   : [GraphicInc.h]
Desc   : グラフィックス関連の共通の型や定数を定義するヘッダーファイル
        これをインクルードすることで、グラフィックス関連のクラスで共通して使用する型や定数を利用できるようにする

------------------------------------------------------------------------------

Date   : 2026/04/19
Author : Yokoyama Haruki

===============================================================================*/

#ifndef _GRAPHIC_INC_H_
#define _GRAPHIC_INC_H_

#include <variant>
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
};
enum class DepthWriteMode : uint8_t
{
    None = 0,
    Write = 1,
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
    Vector3 tangent;

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
