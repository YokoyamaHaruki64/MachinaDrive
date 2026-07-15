#pragma once

#include "System/component.h"

class Camera;
class GameObject;

class Skydome : public Component
{
    Camera* m_mainCamera = nullptr;
    GameObject* m_domeObject = nullptr;
public:
    Skydome() = delete;
    explicit Skydome(GameObject* owner)
        : Component(owner)
    {
    }
    void OnStart() override;
    void Update() override;
};
