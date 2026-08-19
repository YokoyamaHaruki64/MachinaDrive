#include "pch.h"

#include "Scene/GameScene.h"

#include "System/main.h"
#include "System/manager.h"
#include "Component/Transform.h"
#include "Component/SpriteRenderer.h"
#include "Component/Camera.h"
#include "Component/modelRenderer.h"
#include "Component/Skydome.h"
#include "Component/PostEffect.h"
#include "Component/audio.h"
#include "Graphics/Material.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/TextureManager.h"

#include "Script/TItleUI.h"
#include "Script/CameraController.h"
#include "Script/RotateCube.h"
#include "Script/Score.h"

void GameScene::Init()
{
    // シーン初期化
    GameObject* camera = Manager::CreateEmpty(Vector3(0.0f, 2.0f, -5.0f));
    camera->AddComponentForCreate<Camera>(Camera::MainCamera);
    camera->AddComponentForCreate<CameraController>();

    GameObject* field = Manager::CreateEmpty(Vector3::Zero(), Vector3::Zero(), Vector3(10.0f, 1.0f, 10.0f));
    ModelRenderer* fieldRenderer = field->AddComponentForCreate<ModelRenderer>(
        ModelManager::GetPrimitive(ModelManager::PrimitiveType::Plane));
    Material* fieldMaterial = MaterialManager::Get("GameScene.Field.Material");
    if (!fieldMaterial)
    {
        if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile("shader/UnlitTexture.shader"))
        {
            fieldMaterial = MaterialManager::Create("GameScene.Field.Material", *shader);
            if (fieldMaterial)
                fieldMaterial->SetTexture("_MainTex", TextureManager::LoadTexture(L"Assets\\Textures\\glass2.png"));
        }
    }
    fieldRenderer->SetMaterial(0, fieldMaterial);

    GameObject* obj = Manager::CreateEmpty(Vector3(0.0f, 1.0f, -5.0f));
    ModelRenderer* cubeRenderer = obj->AddComponentForCreate<ModelRenderer>(
        ModelManager::GetPrimitive(ModelManager::PrimitiveType::Cube));
    Material* cubeMaterial = MaterialManager::Get("GameScene.Cube.Material");
    if (!cubeMaterial)
    {
        if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile("shader/UnlitTexture.shader"))
        {
            cubeMaterial = MaterialManager::Create("GameScene.Cube.Material", *shader);
            if (cubeMaterial)
                cubeMaterial->SetTexture("_MainTex", TextureManager::LoadTexture(L"Assets\\Textures\\DarkMatReal.png"));
        }
    }
    cubeRenderer->SetMaterial(0, cubeMaterial);
    obj->AddComponentForCreate<RotateCube>();

    Manager::InstantiateForBeginScene(
        "Player",
        Vector3(0.0f, 0.0f, 1.0f),
        Vector3(0.0f, 0.0f, 0.0f),
        Vector3::One());

    const Vector3 enemyPos[] = {
        Vector3(4.0f, 0.0f, 4.0f),
        Vector3(-4.0f, 0.0f, 4.0f),
        Vector3(0.0f, 0.0f, 6.0f),
    };

    for (const auto& pos : enemyPos)
    {
        Manager::InstantiateForBeginScene(
            "Enemy",
            pos,
            Vector3(0.0f, Math::PI, 0.0f), // 敵はプレイヤーと向かい合うように180度回転
            Vector3::One());
    }

    float treePosY = 3.5f; // 木の高さを調整
    const Vector3 treePos[] = {
        Vector3(5.0f, treePosY, 12.0f),
        Vector3(-5.0f, treePosY, 12.0f),
        Vector3(7.0f, treePosY, 8.0f),
        Vector3(-7.0f, treePosY, 8.0f),
    };

    for (const auto& pos : treePos)
    {
        GameObject* tree = Manager::InstantiateForBeginScene(
            "Tree",
            pos,
            Vector3::Zero(),
            Vector3::One() * 5.0f);

        tree->GetComponent<ModelRenderer>()->SetRenderPhase(RenderPhase::Transparent);
    }

    GameObject* skydome = Manager::CreateEmpty();
    skydome->AddComponentForCreate<Skydome>();

    GameObject* scoreObj = Manager::CreateEmpty(Vector3(100.0f,100.0f, 0.0f));
    scoreObj->AddComponentForCreate<Score>();

    Manager::AddPostEffect(std::make_unique<Bloom>());

    Manager::CreateEmpty()->AddComponentForCreate<Audio>("Assets\\Sounds\\bgm.wav")->Play(true);
}
