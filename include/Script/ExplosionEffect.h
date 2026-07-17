#pragma once

#include "System/component.h"

class ExplosionEffect : public Component
{
    static constexpr float LIFETIME = 0.3f; // エフェクトの寿命（秒）
    static constexpr int FRAME_COUNT = 16;   // スプライトシートのフレーム数
    static constexpr float FRAME_DURATION = LIFETIME / FRAME_COUNT; // 1フレームの表示時間
    float m_elapsedTime = 0.0f; // 経過時間
public:
    ExplosionEffect() = delete;
    explicit ExplosionEffect(GameObject* owner)
        : Component(owner)
    {
    }
    void OnStart() override;
    void Update() override;
};
