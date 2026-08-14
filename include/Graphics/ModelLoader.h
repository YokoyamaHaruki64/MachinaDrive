#pragma once

#include "Graphics/ModelAsset.h"

#include <string>

// Assimpを利用して、対応するモデルファイルを直接ModelAssetへ変換する。
class ModelLoader final
{
public:
    ModelLoader() = delete;

    static bool Load(const std::string& filePath, ModelAsset& outModelAsset);
    static void Release(ModelAsset& modelAsset);
};
