#pragma once

#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"

class Camera;
class GameObject;

class Skydome : public Component
{
    REGISTER_COMPONENT(Skydome, "Skydome");

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
