#pragma once

#include "Component/Collider.h"
#include "math/Vector3.h"

class GameObject;
class Rigidbody;

struct HitInfo
{
    bool hit{ false };

    Vector3 normal{ Vector3::Zero() };

    float penetration{ 0.0f };

    Collider* colliderA{ nullptr };
    Collider* colliderB{ nullptr };
    GameObject* objectA{ nullptr };
    GameObject* objectB{ nullptr };
    Rigidbody* rigidbodyA{ nullptr };
    Rigidbody* rigidbodyB{ nullptr };
};

// PhysicsはColliderの組み合わせをその場で判定し、必要な場合だけ解決する。
// Collider一覧や接触状態は保持しないため、呼び出し側が対象を取得して利用する。
// 例えばPlayer側で近傍のColliderを取得してforでCheckCollisionを呼び、hitなら
// ResolveCollisionを呼ぶ。全Objectを走査するPhysicsWorldはこのクラスでは作らない。
class Physics final
{
public:
    static HitInfo CheckCollision(Collider* colliderA, Collider* colliderB);
    static HitInfo CheckCollision(Collider& colliderA, Collider& colliderB)
    {
        return CheckCollision(&colliderA, &colliderB);
    }

    static bool ResolveCollision(const HitInfo& hitInfo);
};
