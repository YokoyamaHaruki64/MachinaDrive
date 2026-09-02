#pragma once

#include "Graphics/GraphicInc.h"

#include <array>
#include <bitset>
#include <d3d11.h>
#include <utility>
#include <vector>
#include <wrl/client.h>

class Material;

// GPU上の共有可能な形状データ。頂点属性はStreamTypeごとの独立した頂点バッファで保持する。
struct Mesh
{
private:
    static constexpr size_t VERTEX_STREAM_COUNT = static_cast<size_t>(VertexStreamType::Count);

    std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, VERTEX_STREAM_COUNT> m_vertexBuffers{};
    std::bitset<VERTEX_STREAM_COUNT> m_vertexStreamMask{};
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer{};

public:
    // ModelAssetのファイル由来Material配列を参照するslot。AssimpのaiMeshは常に一つのMaterialを持つ。
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

        const size_t index = static_cast<size_t>(streamType);
        return m_vertexStreamMask.test(index);
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
        case VertexStreamType::Position:
            return sizeof(PositionVertex);
        case VertexStreamType::NormalTangent:
            return sizeof(NormalTangentVertex);
        case VertexStreamType::PrimaryTexCoord:
            return sizeof(PrimaryTexCoordVertex);
        case VertexStreamType::SecondaryTexCoord:
            return sizeof(SecondaryTexCoordVertex);
        case VertexStreamType::ExtraTexCoord:
            return sizeof(ExtraTexCoordVertex);
        case VertexStreamType::Color:
            return sizeof(ColorVertex);
        default:
            return 0;
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

// ファイルに含まれる階層構造。Meshは共有可能なGPU形状、Nodeはその配置を表す。
struct ModelNode
{
    Matrix4x4 localTransform{ Matrix4x4::Identity() };
    std::vector<uint32_t> meshIndices{};
    std::vector<ModelNode> children{};
};

// ModelLoaderがファイルから直接生成する、ファイル単位のモデルアセット。
// Animation・Skeletonは将来ここへ追加する。
struct ModelAsset
{
    std::vector<Mesh> meshes{};
    std::vector<Material*> embeddedMaterials{};
    ModelNode rootNode{};
};
