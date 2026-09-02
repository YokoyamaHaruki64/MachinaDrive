#include "Component/Skydome.h"

#include "Component/ComponentInc.h"


#include "Graphics/MaterialManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/TextureManager.h"

void Skydome::OnStart()
{
    m_mainCamera = Camera::GetMainCamera();
    if (!m_mainCamera)
    {
        Logger::Error("Skydome: メインカメラが見つかりません。");
        return;
    }
}

void Skydome::Update()
{
    if (m_mainCamera)
    {
        Vector3 cameraPos = m_mainCamera->transform()->GetPosition();
        transform()->SetPosition(cameraPos);
    }
}
