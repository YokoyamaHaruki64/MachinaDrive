#pragma once

#include "System/Component.h"
#include "math/Vector3.h"
#include "math/Curve.h"

#include <vector>

class EffectConfig;

class PatternTrailManager : public Component
{
private:
    std::vector<GameObject*> m_walls;
    EffectConfig* m_config = nullptr;

    float m_elapsedTime = 0.0f; // 経過時間
public:
    PatternTrailManager() = delete;
    explicit PatternTrailManager(GameObject* owner)
        : Component(owner)
    {
    }
    virtual void OnStart() override;
    virtual void Update() override;
    void CreatePatternTrail_Random();

    virtual ~PatternTrailManager() {}
};
