#pragma once

#include "System/Component.h"

#include <vector>

class PatternTrailManager : public Component
{
    std::vector<GameObject*> m_walls;

    float m_spawnInterval = 1.0f; // パターントレイル生成間隔
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
