#pragma once

#include "System/component.h"
#include "math/Curve.h"
#include "Common/Color.h"

class EffectConfig : public Component
{
public:
    struct PatternTrailConfig
    {
        CurveVector3 positionCurve = CurveVector3(
        {
            {0.0f,0.0f,0.0f,0.3f},
            {1.0f,1.0f,1.0f,0.0f}
        },
        {
            {0.0f,0.0f,0.0f,0.0f},
            {1.0f,1.0f,1.0f,0.0f}
        },
        {
            {0.0f,0.0f,0.0f,1.0f},
            {0.0f,1.0f,0.0f,1.0f}
        });

        CurveColor colorCurve = CurveColor(Color::Magenta(), Color(1.0f, 0.0f, 1.0f, 0.0f));
        Curve<float> widthCurve = Curve<float>(1.0f, 0.00f);
        int trailCount = 32;        // パターントレイルの数
        float spawnInterval = 1.0f; // パターントレイルの生成間隔
        float width = 0.1f;         // パターントレイルの幅
        float lifeTime = 1.0f;      // パターントレイルの寿命
        float length = 1.0f;        // パターントレイルの長さ
    };

    struct BoundBallConfig
    {
        float bounciness = 1.0f;  // 反発係数
        float lifetimeMin = 2.0f; // 最小寿命
        float lifetimeMax = 4.0f; // 最大寿命
        float gravity = -9.8f;    // 重力加速度
    };


    EffectConfig() = delete;
    explicit EffectConfig(GameObject* owner)
        : Component(owner)
    {
    }

    void OnStart() override;
    void Update() override;

    const PatternTrailConfig& GetPatternTrailConfig() const { return m_patternTrailConfig; }
    const BoundBallConfig& GetBoundBallConfig() const { return m_boundBallConfig; }

private:

    PatternTrailConfig m_patternTrailConfig;
    PatternTrailConfig m_defaultPatternTrailConfig; // デフォルト設定を保持するメンバ変数
    BoundBallConfig m_boundBallConfig;
    BoundBallConfig m_defaultBoundBallConfig; // デフォルト設定を保持するメンバ変数

    void BoundBallConfigImGui();
    void TrailConfigImGui();

};
