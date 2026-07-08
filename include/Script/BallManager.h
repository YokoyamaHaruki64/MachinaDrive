#pragma once

#include "System/Component.h"

#include <vector>

class BallManager : public Component
{
public:
    struct BoundBallConfig
    {
        float bounciness = 1.0f;  // 反発係数
        float lifetimeMin = 2.0f; // 最小寿命
        float lifetimeMax = 4.0f; // 最大寿命
        float gravity = -9.8f;      // 重力加速度
    };

private:
    static constexpr int MAX_BALL_COUNT = 64; // 最大ボール数

    std::vector<GameObject*> m_walls; // 壁のリスト

    int m_ballCount = 0; // 現在のボール数
    float m_spawnInterval = 2.0f; // ボール生成間隔
    float m_elapsedTime = 0.0f; // 経過時間

    BoundBallConfig m_boundBallConfig; // バウンドボールの設定

public:
    BallManager(GameObject* owner) : Component(owner) {}
    virtual ~BallManager() override {}
    virtual void OnStart() override;
    virtual void Update() override;
    void IncrementBallCount() { m_ballCount++; }
    void DecrementBallCount() { m_ballCount--; }
    int GetBallCount() const { return m_ballCount; }

    void CreateBoll_Random();
    void CreateBoll(Vector3 position, Vector3 normal);

    const BoundBallConfig& GetBoundBallConfig() const { return m_boundBallConfig; }
};
