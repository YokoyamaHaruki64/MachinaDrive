#pragma once

#include <filesystem>

class Scene
{
public:
    virtual ~Scene() = default;

    virtual const std::filesystem::path& GetSceneFilePath() const = 0;
    virtual void Init() = 0;
    virtual void Uninit() {};
    virtual void Update() {};
    virtual void Draw() {};
};
