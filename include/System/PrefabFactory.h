#pragma once

#include "System/GameObject.h"
#include "Graphics/GraphicInc.h"

#include "Component/Transform.h"
#include "Component/modelRenderer.h"
#include "Component/ParticleSystem.h"
#include "Graphics/Material.h"
#include "Graphics/MaterialManager.h"
#include "Graphics/ModelManager.h"
#include "Graphics/Shader/ShaderManager.h"
#include "Graphics/TextureManager.h"

#include "Script/Player.h"
#include "Script/Enemy.h"
#include "Script/Bullet.h"
#include "Script/ExplosionEffect.h"
#include "Script/Ripple.h"
#include "Script/BoundBall.h"
#include "Script/FireworksEffect.h"
#include "Script/ImpactBurst.h"

GameObject* PlayerPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* player = new GameObject(
        position,
        rotation,
        scale
    );
    player->AddComponentForCreate<ModelRenderer>(
        "Assets\\Models\\Player.obj");

    player->AddComponentForCreate<Player>();
    player->AddComponentForCreate<Audio>("Assets\\Sounds\\wan.wav");

    return player;
}

GameObject* EnemyPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* enemy = new GameObject(
        position,
        rotation,
        scale
    );
    enemy->AddComponentForCreate<ModelRenderer>(
        "Assets\\Models\\Player.obj");

    enemy->AddComponentForCreate<Enemy>();

    return enemy;
}

GameObject* BulletPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* bullet = new GameObject(
        position,
        rotation,
        scale
    );
    bullet->AddComponentForCreate<ModelRenderer>(
        "Assets\\Models\\bullet.obj");
    bullet->AddComponentForCreate<Bullet>();
    return bullet;
}

GameObject* TreePrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* tree = new GameObject(
        position,
        rotation,
        scale
    );
    ModelRenderer* renderer = tree->AddComponentForCreate<ModelRenderer>(
        ModelManager::GetPrimitive(ModelManager::PrimitiveType::Quad));
    renderer->SetBillboardType(RendererBase::BillboardType::YAxisFixed);

    // 木は同じShader・Textureを使うため、全インスタンスで専用Materialを共有する。
    Material* material = MaterialManager::Get("Tree.Material");
    if (!material)
    {
        if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile("shader/UnlitTransparent.shader"))
        {
            material = MaterialManager::Create("Tree.Material", *shader);
            if (material)
                material->SetTexture("_MainTex", TextureManager::LoadTexture(L"Assets\\Textures\\tree.png"));
        }
    }

    renderer->SetMaterial(0, material);
    return tree;
}

GameObject* ExplosionPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* explosion = new GameObject(
        position,
        rotation,
        scale
    );
    ModelRenderer* renderer = explosion->AddComponentForCreate<ModelRenderer>(
        ModelManager::GetPrimitive(ModelManager::PrimitiveType::Quad));
    renderer->SetBillboardType(RendererBase::BillboardType::CameraFacing);

    // UVアニメーションがRendererごとに異なるため、ExplosionごとにMaterialを分ける。
    static uint32_t materialInstanceIndex = 0;
    if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile("shader/UnlitTransparent.shader"))
    {
        Material* material = MaterialManager::Create(
            "Explosion.Material." + std::to_string(materialInstanceIndex++),
            *shader);
        if (material)
        {
            material->SetTexture("_MainTex", TextureManager::LoadTexture(L"Assets\\Textures\\explosion.png"));
            renderer->SetMaterial(0, material);
        }
    }

    explosion->AddComponentForCreate<ExplosionEffect>();
    return explosion;
}

GameObject* BoundBallPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* boundBall = new GameObject(
        position,
        rotation,
        scale
    );
    boundBall->AddComponentForCreate<BoundBall>();
    auto p = boundBall->AddComponentForCreate<ParticleEmitter>();
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
    //settings.colorOverLifetime =
    //    Curve<Color>(
    //        Color(0.1f, 0.3f, 0.4f, 1.0f),
    //        Color(0.1f, 0.3f, 0.2f, 0.0f));

    settings.colorOverLifetime = CurveColor(
        Color(0.1f, 0.3f, 0.4f, 1.0f),
        Color(0.1f, 0.3f, 0.2f, 0.0f)
    );
    p->SetSettings(settings);

    boundBall->AddComponentForCreate<ParticleRenderer>(
        p,
        L"Assets/Textures/particle.png");

    return boundBall;
}

GameObject* RipplePrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* ripple = new GameObject(
        position,
        rotation,
        scale
    );
    ModelRenderer* renderer = ripple->AddComponentForCreate<ModelRenderer>(
        ModelManager::GetPrimitive(ModelManager::PrimitiveType::Quad));
    renderer->SetRenderPhase(RenderPhase::Transparent);

    // 波紋は色を個別更新するため、インスタンスごとにMaterialを生成する。
    static uint32_t materialInstanceIndex = 0;
    if (GraphicsShader* shader = ShaderManager::LoadShaderFromFile("shader/UnlitAdditive.shader"))
    {
        Material* material = MaterialManager::Create(
            "Ripple.Material." + std::to_string(materialInstanceIndex++),
            *shader);
        if (material)
        {
            material->SetTexture("_MainTex", TextureManager::LoadTexture(L"Assets\\Textures\\Ripple.png"));
            renderer->SetMaterial(0, material);
        }
    }
    ripple->AddComponentForCreate<Ripple>();
    return ripple;
}

GameObject* FireworksEffectPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* fireworks = new GameObject(
        position,
        rotation,
        scale
    );
    auto p = fireworks->AddComponentForCreate<ParticleEmitter>();
    fireworks->AddComponentForCreate<ParticleRenderer>(
        p,
        L"Assets/Textures/particle.png");
    fireworks->AddComponentForCreate<FireworksEffect>();
    return fireworks;
}

GameObject* ImpactBurstEffectPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* impact = new GameObject(
        position,
        rotation,
        scale
    );
    auto p = impact->AddComponentForCreate<ParticleEmitter>();
    impact->AddComponentForCreate<ParticleRenderer>(
        p,
        L"Assets/Textures/particle.png");
    impact->AddComponentForCreate<ImpactBurst>();
    return impact;
}
