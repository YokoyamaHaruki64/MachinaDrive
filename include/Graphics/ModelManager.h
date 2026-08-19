#pragma once

#include "Graphics/ModelAsset.h"

#include <string>

// ファイル由来とランタイム生成のModelAssetを一元管理する。
class ModelManager final
{
public:
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
};
