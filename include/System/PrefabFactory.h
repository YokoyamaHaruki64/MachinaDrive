#pragma once

#include "System/GameObject.h"

#include "Component/Transform.h"
#include "Component/modelRenderer.h"

#include "Script/Player.h"

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
        "BuildedShader\\unlitTexturePS.cso");

    return enemy;
}
