#pragma once

class Scene
{
public:
    virtual void Init() = 0;
    virtual void Uninit() {};
    virtual void Update() {};
    virtual void Draw() {};
};
