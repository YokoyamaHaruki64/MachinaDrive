#pragma once

#include "System/Component.h"
#include "math/Vector3.h"
#include "math/Curve.h"

#include <vector>

class PatternTrailManager : public Component
{
public:
    struct PatternTrailSettings
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
        Curve<float> widthCurve = Curve<float>(0.2f, 0.05f);
        float width = 0.1f; // パターントレイルの幅
        float lifeTime = 1.0f;  // パターントレイルの寿命
        float length = 1.0f;    // パターントレイルの長さ
    };
private:
    std::vector<GameObject*> m_walls;
    PatternTrailSettings m_settings;
    PatternTrailSettings m_defaultSettings; // デフォルト設定を保持するメンバ変数

    float m_spawnInterval = 1.0f; // パターントレイル生成間隔
    float m_elapsedTime = 0.0f; // 経過時間
    int m_trailCount = 32; // 生成するパターントレイルの数
public:
    PatternTrailManager() = delete;
    explicit PatternTrailManager(GameObject* owner)
        : Component(owner)
    {
    }
    virtual void OnStart() override;
    virtual void Update() override;
    void CreatePatternTrail_Random();
    const PatternTrailSettings& GetSettings() const { return m_settings; }

    virtual ~PatternTrailManager() {}
};
