#pragma once

#include "System/Core/Script.h"
#include "System/Core/GameObjectReference.h"

class Camera;

class CameraController : public Script
{
    GM_SCRIPT(CameraController, "Camera Controller");
    USE_FIELDS

    GameObjectReference m_target;
    Camera* m_camera = nullptr;

    float m_distance = 10.0f;
    float m_sensitivity = 0.1f;

    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    bool isLocked = false;
public:
    void SetTarget(GameObject* target) { m_target = target; }
    GameObject* GetTarget() const { return m_target.Get(); }

    void BeginLock() { isLocked = true; }
    void EndLock() { isLocked = false; }
    bool IsLocked() const { return isLocked; }


    void OnStart() override;
    void LateUpdate() override;
    void OnDestroy() override;
};
