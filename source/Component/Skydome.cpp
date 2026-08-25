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
        MessageBoxA(nullptr, "Skydome: Main Camera not found!", "Error", MB_OK | MB_ICONERROR);
    }

    m_domeObject = Manager::CreateEmpty(Vector3::Zero(), Vector3::Zero(), Vector3(1000.0f, 1000.0f, 1000.0f));
    auto mr = m_domeObject->AddComponent<ModelRenderer>(
        "Assets\\Local\\Models\\sky.obj");

    Material* skydomeMaterial = MaterialManager::Get("Skydome.Material");
    if(!skydomeMaterial)
    {
        if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile("Assets/Shaders/UnlitTexture.shader"))
        {
            skydomeMaterial = MaterialManager::Create("Skydome.Material", *shader);
            if (skydomeMaterial)
                skydomeMaterial->SetTexture("_MainTex", TextureManager::LoadTexture(L"Assets\\Local\\Textures\\sky.jpg"));
        }
    }
}

void Skydome::Update()
{
    if (m_mainCamera && m_domeObject)
    {
        Vector3 cameraPos = m_mainCamera->transform()->GetPosition();
        m_domeObject->GetTransform()->SetPosition(cameraPos);
    }
}
