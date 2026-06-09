#include "pch.h"

#include "Component/Camera.h"
#include "System/Renderer.h"
#include "Component/Transform.h"

std::unordered_map<Camera::CameraTag, Camera*> Camera::s_camerasByTag;
Camera* Camera::s_mainCamera = nullptr;

Camera* Camera::GetMainCamera()
{
    return s_mainCamera ? s_mainCamera : GetCameraByTag(CameraTag::MainCamera);
}

Camera* Camera::GetCameraByTag(CameraTag tag)
{
    auto it = s_camerasByTag.find(tag);
    return it != s_camerasByTag.end() ? it->second : nullptr;
}

void Camera::SetViewProjection()
{
    XMMATRIX viewMatrix = XMMatrixLookAtLH(
        XMLoadFloat3((XMFLOAT3*)&transform()->GetPosition()),
        XMLoadFloat3((XMFLOAT3*)&m_target),
        XMLoadFloat3((XMFLOAT3*)&m_up)
    );

    XMMATRIX projMatrix =
        XMMatrixPerspectiveFovLH(1.0f, static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 0.1f, 1000.0f);

    Renderer::SetViewMatrix(viewMatrix);
    Renderer::SetProjectionMatrix(projMatrix);

}
