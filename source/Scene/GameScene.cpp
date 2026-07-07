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
#include "Script/BoundBall.h"
#include "Component/ParticleSystem.h"


void GameScene::Init()
{
    // シーン初期化
    GameObject* camera = Manager::CreateEmpty(Vector3(0.0f, 2.0f, -5.0f));
    camera->AddComponentForBeginScene<Camera>(Camera::MainCamera);
    camera->AddComponentForBeginScene<CameraController>();

    // const float BOX_SIZE = 5.0f;
    //const float SIZE_HALF = BOX_SIZE * 0.5f;
    const float SIZE_Y = 6.0f;
    const float SIZE_XZ = 6.0f;
    const float THICKNESS = 0.1f;

    const float HEIGHT_OFFSET = 2.0f;

    auto CreateWall = [](const Vector3& position, const Vector3& scale, const Vector3& normal)
    {
        GameObject* wall = Manager::CreateEmpty(position, Vector3::Zero(), scale);
        wall->AddComponentForBeginScene<Wall>()->SetNormal(normal);
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

    GameObject* ball = Manager::CreateEmpty(Vector3(1.0f,1.0f, 0.0f));
    auto* boundBall = ball->AddComponentForBeginScene<BoundBall>();
    auto* p = ball->AddComponentForBeginScene<ParticleEmitter>();
    ParticleEmitter::EmitterSettings settings = p->GetSettings();
    settings.burst = false;
    settings.maxParticles = 100;
    settings.lifetimeMin = 0.05f;
    settings.lifetimeMax = 0.05f;
    settings.gravity = Vector3(0.0f, 0.0f, 0.0f);
    settings.emitRate = 1000;
    settings.velocityType = ParticleEmitter::VelocityType::None;
    settings.spawnType = ParticleEmitter::SpawnType::Point;
    settings.randomizeSpeed = true;
    settings.speedMin = 3.0f;
    settings.speedMax = 5.0f;
    settings.scaleOverLifetime =
        Curve<float>(
            1.0f,
            1.0f);
    settings.colorOverLifetime =
        Curve<Color>(
            Color(0.1f, 0.3f, 0.4f, 1.0f),
            Color(0.1f, 0.3f, 0.2f, 0.0f));
    p->SetSettings(settings);
    ball->AddComponentForBeginScene<ParticleRenderer>(
        p,
        L"Assets/Textures/Particle.png");
    //ball->AddComponentForBeginScene<Cube>(
    //    L"Assets/Textures/DarkMatReal.png",
    //    "BuildedShader/Dice_VS.cso",
    //    "BuildedShader/Dice_PS.cso");
    boundBall->SetVelocity(Vector3(1.0, 1.5f, 1.0f) * 5.0f);
    
}
