#pragma once

#include "System/Script.h"
#include "System/GameObjectReference.h"

class Camera;

class CameraController : public Script
{
    GameObjectReference m_target;
    Camera* m_camera = nullptr;

    static constexpr float CAMERA_DISTANCE = 10.0f;
    static constexpr float CAMERA_HEIGHT = 2.0f;
public:
    CameraController() = default;
    explicit CameraController(GameObject* owner)
        : Script(owner)
    {
    }
    void SetTarget(GameObject* target) { m_target = target; }
    GameObject* GetTarget() const { return m_target.Get(); }

    void OnStart() override;
    void LateUpdate() override;
};
