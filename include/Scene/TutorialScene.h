#pragma once

#include "Scene/Scene.h"

class TutorialScene final : public Scene
{
    inline static const std::filesystem::path s_sceneFilePath{ "Assets/Scenes/Tutorial.scene" };

public:
    const std::filesystem::path& GetSceneFilePath() const override { return s_sceneFilePath; }
    void Init() override;
};
