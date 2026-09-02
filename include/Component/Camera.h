#pragma once

#include "math/Vector3.h"
#include "System/Core/component.h"
#include "System/Core/ComponentRegistry.h"
#include "System/Core/GameObject.h"

#include <unordered_map>

class Camera : public Component
{
    REGISTER_COMPONENT(Camera, "Camera");

public:

    enum CameraTag
    {
        MainCamera,
        SubCamera,
        EditorCamera,
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
            s_sceneMainCamera = this;
        }
        s_camerasByTag[tag] = this;
    }
    ~Camera()
    {
        // Sceneカメラと現在の描画カメラの登録を、破棄対象だけ解除する。
        if (s_sceneMainCamera == this)
        {
            s_sceneMainCamera = nullptr;
        }
        if (s_activeMainCamera == this)
        {
            s_activeMainCamera = nullptr;
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
    static Camera* GetSceneMainCamera();
    static Camera* GetCameraByTag(CameraTag tag);
    static void SetActiveMainCamera(Camera* camera);

private:
    Vector3 m_target{ 0.0f, 0.5f, 0.0f };
    Vector3 m_up{ 0.0f, 1.0f, 0.0f };

    static Camera* s_sceneMainCamera;
    static Camera* s_activeMainCamera;
    static std::unordered_map<CameraTag, Camera*> s_camerasByTag;

protected:
    void RegisterFields(FieldRegistry& fields) override
    {
        GM_FIELD(m_target);
        GM_FIELD(m_up);
    }

};
