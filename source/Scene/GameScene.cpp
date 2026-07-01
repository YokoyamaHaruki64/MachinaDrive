#include "pch.h"

#include "Scene/GameScene.h"

#include "System/main.h"
#include "System/manager.h"
#include "Component/Transform.h"
#include "Component/SpriteRenderer.h"
#include "Component/Camera.h"
#include "Component/Primitive.h"

#include "Script/TItleUI.h"
#include "Script/CameraController.h"
#include "Script/RotateCube.h"


void GameScene::Init()
{
    // シーン初期化
    GameObject* camera = Manager::CreateEmpty(Vector3(0.0f, 2.0f, -5.0f));
    camera->AddComponentForBeginScene<Camera>(Camera::MainCamera);
    camera->AddComponentForBeginScene<CameraController>();

    GameObject* field = Manager::CreateEmpty(Vector3::Zero(), Vector3::Zero(), Vector3(10.0f, 1.0f, 10.0f));
    field->AddComponentForBeginScene<Plane>(
        L"Assets\\Textures\\glass2.png",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso");

    GameObject* obj = Manager::CreateEmpty(Vector3(0.0f, 1.0f, -5.0f));
    obj->AddComponentForBeginScene<Cube>(
        L"Assets\\Textures\\DarkMatReal.png",
        "BuildedShader\\Dice_VS.cso",
        "BuildedShader\\Dice_PS.cso");
    obj->AddComponentForBeginScene<RotateCube>();

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

        tree->GetComponent<Quad>()->SetRendererPass(RenderPass::Transparent);
    }
}
