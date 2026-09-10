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
    float m_sensitivity = 0.05f;
    Vector3 m_offset = Vector3(0.0f, 1.0f, 0.0f);
    float m_collisionPadding = 0.3f;

    float m_yaw = Math::PI;
    float m_pitch = 0.0f;

    bool m_isShaking = false;
    float m_shakeDuration = 0.0f;
    float m_shakeTimer = 0.0f;
    float m_shakeMagnitude = 0.0f;
    Vector3 m_shakeOffset{ 0.0f, 0.0f, 0.0f };

    bool isLocked = false;

    Vector3 ResolveWallCollision(
        const Vector3& anchorPosition,
        const Vector3& desiredPosition,
        const Vector3& fallbackPosition) const;


public:
    void SetTarget(GameObject* target) { m_target = target; }
    GameObject* GetTarget() const { return m_target.Get(); }

    void BeginLock() { isLocked = true; }
    void EndLock() { isLocked = false; }
    bool IsLocked() const { return isLocked; }

    void Shake(float duration, float magnitude);

    void OnStart() override;
    void LateUpdate() override;
};
