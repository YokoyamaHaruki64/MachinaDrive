#include "pch.h"

#include "Scene/GameScene.h"

#include "System/main.h"
#include "System/manager.h"
#include "Component/Transform.h"
#include "Component/SpriteRenderer.h"
#include "Component/Camera.h"
#include "Component/Primitive.h"

#include "Script/CameraController.h"

#include "Script/Wall.h"
#include "Script/BallManager.h"
#include "Script/PatternTrailManager.h"

#include "Common/Random.h"

void GameScene::Init()
{
    // シーン初期化
    GameObject* camera = Manager::CreateEmpty(Vector3(0.0f, 2.0f, -5.0f));
    camera->AddComponentForCreate<Camera>(Camera::MainCamera);
    camera->AddComponentForCreate<CameraController>();

    // const float BOX_SIZE = 5.0f;
    //const float SIZE_HALF = BOX_SIZE * 0.5f;
    const float SIZE_Y = 6.0f;
    const float SIZE_XZ = 6.0f;
    const float THICKNESS = 0.1f;

    const float HEIGHT_OFFSET = 2.2f;

    auto CreateWall = [](const Vector3& position, const Vector3& scale, const Vector3& normal)
    {
        GameObject* wall = Manager::CreateEmpty(position, Vector3::Zero(), scale);
        wall->AddComponentForCreate<Wall>()->SetNormal(normal);
        auto cube = wall->AddComponentForCreate<Cube>(L"Assets/Textures/DarkMatReal.png");
        cube->SetRenderPass(RenderPass::Transparent);
        cube->SetBlendMode(BlendMode::AlphaBlend);
        cube->SetMaterial({
            {0.2f, 0.2f, 0.2f, 1.0f}, // Ambient
            {1.0f, 1.0f, 1.0f, 0.02f}, // Diffuse
            {1.0f, 1.0f, 1.0f, 1.0f}, // Specular
            {0.0f, 0.0f, 0.0f, 1.0f}, // Emission
            32.0f,                    // Shininess
            TRUE                      // TextureEnable
            });
    };

    // 床
    CreateWall(
        Vector3(0.0f, -HEIGHT_OFFSET + THICKNESS * 0.5f, 0.0f),
        Vector3(SIZE_XZ, THICKNESS, SIZE_XZ),
        Vector3::Up());
    // 左の壁
    CreateWall(
        Vector3(-SIZE_XZ * 0.5f + THICKNESS * 0.5f, -HEIGHT_OFFSET + SIZE_Y * 0.5f, 0.0f),
        Vector3(THICKNESS, SIZE_Y, SIZE_XZ),
        Vector3::Right());
    // 右の壁
    CreateWall(
        Vector3(SIZE_XZ * 0.5f - THICKNESS * 0.5f, -HEIGHT_OFFSET + SIZE_Y * 0.5f, 0.0f),
        Vector3(THICKNESS, SIZE_Y, SIZE_XZ),
        Vector3::Left());
    // 手前の壁
    CreateWall(
        Vector3(0.0f, -HEIGHT_OFFSET + SIZE_Y * 0.5f, -SIZE_XZ * 0.5f + THICKNESS * 0.5f),
        Vector3(SIZE_XZ, SIZE_Y, THICKNESS),
        Vector3::Forward());
    // 奥の壁
    CreateWall(
        Vector3(0.0f, -HEIGHT_OFFSET + SIZE_Y * 0.5f, SIZE_XZ * 0.5f - THICKNESS * 0.5f),
        Vector3(SIZE_XZ, SIZE_Y, THICKNESS),
        Vector3::Backward());
    //// 天井
    CreateWall(
        Vector3(0.0f, -HEIGHT_OFFSET + SIZE_Y - THICKNESS * 0.5f, 0.0f),
        Vector3(SIZE_XZ, THICKNESS, SIZE_XZ),
        Vector3::Down());

    GameObject* ballManager = Manager::CreateEmpty(Vector3::Zero());
    ballManager->AddComponentForCreate<BallManager>();

    GameObject* patternTrailManager = Manager::CreateEmpty(Vector3::Zero());
    patternTrailManager->AddComponentForCreate<PatternTrailManager>();
}
