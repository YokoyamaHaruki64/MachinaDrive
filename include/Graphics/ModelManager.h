#pragma once

#include "Graphics/ModelAsset.h"

#include <string>
#include <optional>
#include <vector>

// ファイル由来とランタイム生成のModelAssetを一元管理する。
class ModelManager final
{
public:
    // ランタイムでGPU Meshを生成するための頂点ストリーム。空のストリームは作成しない。
    struct RuntimeMeshData
    {
        std::vector<PositionVertex> positions{};
        std::vector<NormalTangentVertex> normalTangents{};
        std::vector<PrimaryTexCoordVertex> primaryTexCoords{};
        std::vector<SecondaryTexCoordVertex> secondaryTexCoords{};
        std::vector<ExtraTexCoordVertex> extraTexCoords{};
        std::vector<ColorVertex> colors{};
        std::vector<uint32_t> indices{};
    };

    enum class PrimitiveType
    {
        Quad,
        Plane,
        Cube
    };

    ModelManager() = delete;

    static void Init();
    static void Uninit();

    static ModelAsset* Load(const std::string& filePath);
    static ModelAsset* GetPrimitive(PrimitiveType primitiveType);
    // assetNameごとに一度だけ登録する。戻り値はModelManagerのUninitまで有効。
    static ModelAsset* CreateRuntimeModel(
        const std::string& assetName,
        const RuntimeMeshData& meshData,
        Material* material = nullptr);
    static std::optional<std::string> GetSourcePath(const ModelAsset* modelAsset);
};
