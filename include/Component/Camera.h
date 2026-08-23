#pragma once

#include "math/Vector3.h"
#include "System/Component.h"
#include "System/ComponentRegistry.h"
#include "System/GameObject.h"

#include <unordered_map>

class Camera : public Component
{
    REGISTER_COMPONENT(Camera, "Camera");

public:

    enum CameraTag
    {
        MainCamera,
        SubCamera
    };

    void SetTarget(const Vector3& target) { m_target = target; }
    void SetUp(const Vector3& up) { m_up = up; }

    Vector3 GetTarget() const { return m_target; }
    Vector3 GetUp() const { return m_up; }

    void SetViewProjection();

    Camera() = default;
    Camera(GameObject* owner, CameraTag tag = CameraTag::MainCamera)
        : Component(owner)
    {
        if (tag == CameraTag::MainCamera)
        {
            s_mainCamera = this;
        }
        s_camerasByTag[tag] = this;
    }
    ~Camera()
    {
        // カメラが破棄されるときに、メインカメラやタグ付きカメラの登録を解除する
        if (s_mainCamera == this)
        {
            s_mainCamera = nullptr;
        }
        for (auto it = s_camerasByTag.begin(); it != s_camerasByTag.end(); ++it)
        {
            if (it->second == this)
            {
                s_camerasByTag.erase(it);
                break;
            }
        }
    }
    static Camera* GetMainCamera();
    static Camera* GetCameraByTag(CameraTag tag);

private:
    Vector3 m_target{ 0.0f, 0.5f, 0.0f };
    Vector3 m_up{ 0.0f, 1.0f, 0.0f };

    static Camera* s_mainCamera;
    static std::unordered_map<CameraTag, Camera*> s_camerasByTag;

};
