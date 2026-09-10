#include "pch.h"

#include "Component/ComponentInc.h"


#include "Component/Camera.h"
#include "System/Rendering/renderer.h"

std::unordered_map<Camera::CameraTag, Camera*> Camera::s_camerasByTag;
Camera* Camera::s_sceneMainCamera = nullptr;
Camera* Camera::s_activeMainCamera = nullptr;

Camera* Camera::GetMainCamera()
{
    return s_activeMainCamera ? s_activeMainCamera : s_sceneMainCamera;
}

Camera* Camera::GetSceneMainCamera()
{
    return s_sceneMainCamera;
}

Camera* Camera::GetCameraByTag(CameraTag tag)
{
    auto it = s_camerasByTag.find(tag);
    return it != s_camerasByTag.end() ? it->second : nullptr;
}

void Camera::SetActiveMainCamera(Camera* camera)
{
    s_activeMainCamera = camera;
}

void Camera::SetViewProjection()
{
    const Vector3 eyePosition = transform()->GetPosition();
    XMMATRIX viewMatrix = XMMatrixLookAtLH(
        XMLoadFloat3((XMFLOAT3*)&eyePosition),
        XMLoadFloat3((XMFLOAT3*)&m_target),
        XMLoadFloat3((XMFLOAT3*)&m_up)
    );

    const float renderWidth = static_cast<float>(Renderer::GetGameWidth());
    const float renderHeight = static_cast<float>(Renderer::GetGameHeight());
    const float aspectRatio = renderHeight > 0.0f
        ? renderWidth / renderHeight
        : static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(1.0f, aspectRatio, 0.1f, 1000.0f);

    Renderer::SetViewMatrix(viewMatrix);
    Renderer::SetProjectionMatrix(projMatrix);
    Renderer::SetCameraPosition(eyePosition);

}
