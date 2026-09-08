#pragma once

#include "Graphics/GraphicInc.h"

#include <vector>
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
