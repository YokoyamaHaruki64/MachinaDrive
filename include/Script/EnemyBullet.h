#pragma once

#include "System/Core/Script.h"
#include "math/Curve.h"

class EnemyBullet final : public Script
{
    GM_SCRIPT(EnemyBullet, "Enemy Bullet");
    USE_FIELDS
    static constexpr float LIFE_TIME = 5.0f; // 弾の寿命（秒）

    Vector3 m_velocity = Vector3::Zero();
    float m_elapsedTime = 0.0f; // 経過時間
    Curve<float> m_trailWidthCurve{ 1.0f, 0.0f };
    CurveColor m_trailColorCurve{
        Color(1.0f, 0.3f, 0.1f, 1.0f),
        Color(1.0f, 0.3f, 0.1f, 0.0f) };

    class TrailRenderer* m_trailRenderer = nullptr;

    void ApplyTrailSettings();

public:
    void OnStart() override;
    void Update() override;
    void OnHitBegin(const HitInfo& hitInfo) override;

    void SetVelocity(const Vector3& velocity)
    {
        m_velocity = velocity;
    }

    void SetTrailWidthCurve(const Curve<float>& curve);
    void SetTrailColorCurve(const CurveColor& curve);
};
