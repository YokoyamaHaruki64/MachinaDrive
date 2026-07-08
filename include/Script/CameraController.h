#pragma once

#include "System/Component.h"

class Transform;
class Camera;

class CameraController : public Component
{
    Transform* m_target = nullptr;
    Camera* m_camera = nullptr;

    static constexpr float CAMERA_DISTANCE = 10.0f;
    static constexpr float CAMERA_HEIGHT = 2.0f;
public:
    CameraController() = default;
    explicit CameraController(GameObject* owner)
        : Component(owner)
    {
    }
    void SetTarget(Transform* target) { m_target = target; }
    Transform* GetTarget() const { return m_target; }

    void OnStart() override;
    void LateUpdate() override;
};
