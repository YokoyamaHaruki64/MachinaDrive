#include "pch.h"

#include "System/GM_Time.h"
#include "System/manager.h"
#include "Component/Transform.h"

#include "Script/BoundBall.h"
#include "Script/Wall.h"

void BoundBall::Update()
{
    float dt = Time::GetDeltaTime();
    Vector3 pos = transform()->GetPosition();
    Vector3 prevPos = pos;

    // 位置の更新
    m_velocity.y -= 9.81f * dt; // 重力加速度を適用
    pos += m_velocity * dt;


    // 壁との衝突判定(簡易)
    auto walls = Manager::GetObjectsByComponent<Wall>();

    for( auto wallObj : walls)
    {
        Wall* wall = wallObj->GetComponent<Wall>();
        if (!wall) continue;

        // 壁の位置とサイズを取得
        Vector3 wallPos = wall->transform()->GetPosition();
        Vector3 wallScale = wall->transform()->GetScale();

        // 衝突判定
        Vector3 minBound = wallPos - wallScale * 0.5f;
        Vector3 maxBound = wallPos + wallScale * 0.5f;

        // 最近点を計算
        Vector3 closestPoint = Vector3(
            Math::Clamp(pos.x, minBound.x, maxBound.x),
            Math::Clamp(pos.y, minBound.y, maxBound.y),
            Math::Clamp(pos.z, minBound.z, maxBound.z)
        );

        // 衝突判定
        // 球と最近点の距離/半径を比較
        float distSq = Vector3::DistanceSqr(pos, closestPoint);

        // 距離<半径で衝突
        if (distSq < m_radius * m_radius)
        {
            Vector3 normal = wall->GetNormal();

            // 反射ベクトルの計算
            m_velocity = Vector3::Reflect(m_velocity, normal, m_bounciness);
            pos = closestPoint + normal * m_radius; // 衝突後の位置を修正
        }

    }

    transform()->SetPosition(pos);


}
