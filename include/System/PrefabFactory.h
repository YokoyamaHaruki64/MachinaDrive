#pragma once

#include "System/GameObject.h"

#include "Component/Transform.h"
#include "Component/modelRenderer.h"
#include "Component/Primitive.h"
#include "Component/ParticleSystem.h"

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
        "Assets\\Models\\Player.obj",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso",
        ModelRenderer::InverseAxis::Y); // Y軸反転

    player->AddComponentForCreate<Player>();

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
        "Assets\\Models\\Player.obj",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso",
        ModelRenderer::InverseAxis::Y); // Y軸反転

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
        "Assets\\Models\\bullet.obj",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso"); // Y軸反転
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
    tree->AddComponentForCreate<Quad>(
        L"Assets\\Textures\\tree.png",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso")->SetBillboardType(RendererBase::BillboardType::YAxisFixed);
    return tree;
}

GameObject* ExplosionPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* explosion = new GameObject(
        position,
        rotation,
        scale
    );
    explosion->AddComponentForCreate<Quad>(
        L"Assets\\Textures\\explosion.png",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso")->SetBillboardType(RendererBase::BillboardType::CameraFacing);

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
    settings.colorOverLifetime =
        Curve<Color>(
            Color(0.1f, 0.3f, 0.4f, 1.0f),
            Color(0.1f, 0.3f, 0.2f, 0.0f));
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
    auto q = ripple->AddComponentForCreate<Quad>(
        L"Assets\\Textures\\Ripple.png",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso");
    q->SetBlendMode(BlendMode::Additive);
    q->SetRenderPass(RenderPass::Transparent);
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
