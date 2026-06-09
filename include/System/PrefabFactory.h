#pragma once

#include "System/GameObject.h"

#include "Component/Transform.h"
#include "Component/modelRenderer.h"
#include "Component/Primitive.h"

#include "Script/Player.h"
#include "Script/Enemy.h"
#include "Script/Bullet.h"
#include "Script/ExplosionEffect.h"

GameObject* PlayerPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* player = new GameObject(
        position,
        rotation,
        scale
    );
    player->AddComponent<ModelRenderer>(
        "Assets\\Models\\Player.obj",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso",
        ModelRenderer::InverseAxis::Y); // Y軸反転

    player->AddComponent<Player>();

    return player;
}

GameObject* EnemyPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* enemy = new GameObject(
        position,
        rotation,
        scale
    );
    enemy->AddComponent<ModelRenderer>(
        "Assets\\Models\\Player.obj",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso",
        ModelRenderer::InverseAxis::Y); // Y軸反転

    enemy->AddComponent<Enemy>();

    return enemy;
}

GameObject* BulletPrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* bullet = new GameObject(
        position,
        rotation,
        scale
    );
    bullet->AddComponent<ModelRenderer>(
        "Assets\\Models\\bullet.obj",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso"); // Y軸反転
    bullet->AddComponent<Bullet>();
    return bullet;
}

GameObject* TreePrefabFactory(const Vector3& position, const Vector3& rotation, const Vector3& scale)
{
    GameObject* tree = new GameObject(
        position,
        rotation,
        scale
    );
    tree->AddComponent<Quad>(
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
    explosion->AddComponent<Quad>(
        L"Assets\\Textures\\explosion.png",
        "BuildedShader\\unlitTextureVS.cso",
        "BuildedShader\\unlitTexturePS.cso")->SetBillboardType(RendererBase::BillboardType::CameraFacing);

    explosion->AddComponent<ExplosionEffect>();
    return explosion;
}
