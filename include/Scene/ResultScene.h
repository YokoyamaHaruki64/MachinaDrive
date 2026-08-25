#pragma once
#include "Scene/Scene.h"

class ResultScene : public Scene
{
    inline static const std::filesystem::path s_sceneFilePath{ "Assets/Scenes/Result.scene" };

public:
    const std::filesystem::path& GetSceneFilePath() const override { return s_sceneFilePath; }
    void Init() override;
};
