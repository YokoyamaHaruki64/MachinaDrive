#include "pch.h"

#include "System/GM_Time.h"
#include "System/manager.h"
#include "Common/Random.h"
#include "Component/Transform.h"
#include "Component/ParticleSystem.h"

#include "Script/BoundBall.h"
#include "Script/Wall.h"
#include "Script/BallManager.h"


void BoundBall::CreateRippleEffect(Vector3 normal, Vector3 position)
{
    GameObject* ripple = Manager::Instantiate(
        "Ripple",
        position,
        Vector3::Zero(),
        Vector3(1.0f, 1.0f, 1.0f)
    );

    Transform* rippleTransform = ripple->GetComponent<Transform>();
    if (!rippleTransform) return;

    Vector3 n = normal.Normalized();

    float pitch = std::asin(std::clamp(n.y, -1.0f, 1.0f));
    float yaw = std::atan2(-n.x, -n.z);

    rippleTransform->SetRotation(Vector3(pitch, yaw, 0.0f));

}

void BoundBall::OnStart()
{
    m_ballManager = Manager::GetFirstObjectByComponent<BallManager>()->GetComponent<BallManager>();
    BallManager::BoundBallConfig config = m_ballManager->GetBoundBallConfig();
    m_lifetime = Random::Range(config.lifetimeMin, config.lifetimeMax);
    m_elapsedTime = 0.0f;
}

void BoundBall::Update()
{
    BallManager::BoundBallConfig config = m_ballManager->GetBoundBallConfig();
    float dt = Time::GetDeltaTime();
    m_elapsedTime += dt;
    Vector3 pos = transform()->GetPosition();
    Vector3 prevPos = pos;

    // 位置の更新
    m_velocity.y -= config.gravity * dt; // 重力加速度を適用
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
        if (distSq < RADIUS * RADIUS)
        {
            Vector3 normal = wall->GetNormal();

            // 反射ベクトルの計算
            m_velocity = Vector3::Reflect(m_velocity, normal);
            pos = closestPoint + normal * RADIUS; // 衝突後の位置を修正

            // Rippleエフェクトの生成
            CreateRippleEffect(normal, closestPoint);
            auto e = Manager::Instantiate(
                "ImpactBurst",
                closestPoint,
                Vector3::Zero(),
                Vector3(1.0f, 1.0f, 1.0f)
            );
            auto emi = e->GetComponent<ParticleEmitter>();
            if (emi)
            {
                auto settings = emi->GetSettings();
                settings.direction = normal * -1.0f;
                emi->SetSettings(settings);
            }
            m_boundCount++;
        }

    }

    if (m_elapsedTime >= m_lifetime ||
        m_velocity.LengthSqr() < 10.0f ||
        m_boundCount >= MAX_BOUND_COUNT)
    {
        //Manager::Instantiate(
        //    "ExplosionEffect",
        //    pos,
        //    Vector3::Zero(),
        //    Vector3(1.0f, 1.0f, 1.0f)
        //);

        Vector3 p = Random::InsideUnitSphere() * 2.0f + Vector3(0.0f, 1.0f, 0.0f);
        Vector3 v = Random::OnUnitSphere() * 10.0f;

        Manager::Instantiate(
            "Firework",
            pos,
            Vector3::Zero(),
            Vector3(1.0f, 1.0f, 1.0f)
        );
        m_ballManager->DecrementBallCount();
        m_owner->Destroy();
        return;

    }
    transform()->SetPosition(pos);

}
