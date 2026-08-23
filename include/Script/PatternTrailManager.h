#pragma once

#include "System/Script.h"
#include "System/GameObjectReference.h"
#include "math/Vector3.h"
#include "math/Curve.h"

#include <vector>

class EffectConfig;

class PatternTrailManager : public Script
{
private:
    std::vector<GameObjectReference> m_walls;
    EffectConfig* m_config = nullptr;

    float m_elapsedTime = 0.0f; // 経過時間
public:
    PatternTrailManager() = delete;
    explicit PatternTrailManager(GameObject* owner)
        : Script(owner)
    {
    }
    virtual void OnStart() override;
    virtual void Update() override;
    void CreatePatternTrail_Random();

    virtual ~PatternTrailManager() {}
};
