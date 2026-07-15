#include "Component/Skydome.h"

#include "Component/Transform.h"
#include "Component/modelRenderer.h"
#include "Component/Camera.h"
#include "System/manager.h"

void Skydome::OnStart()
{
    m_mainCamera = Camera::GetMainCamera();
    if (!m_mainCamera)
    {
        MessageBoxA(nullptr, "Skydome: Main Camera not found!", "Error", MB_OK | MB_ICONERROR);
    }

    GameObject* domeObject = Manager::CreateEmpty(Vector3::Zero(), Vector3::Zero(), Vector3(1000.0f, 1000.0f, 1000.0f));
    domeObject->AddComponent<ModelRenderer>(
        "Assets\\Models\\sky.obj",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso");
}

void Skydome::Update()
{
    if (m_mainCamera && m_domeObject)
    {
        Vector3 cameraPos = m_mainCamera->transform()->GetPosition();
        m_domeObject->GetTransform()->SetPosition(cameraPos);
    }
}
