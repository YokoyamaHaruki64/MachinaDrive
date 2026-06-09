#pragma once

#include "System/component.h"

class ExplosionEffect : public Component
{
    static constexpr float lifetime = 0.2f; // エフェクトの寿命（秒）
    static constexpr int frameCount = 16;   // スプライトシートのフレーム数
    static constexpr float frameDuration = lifetime / frameCount; // 1フレームあたりの表示時間
    float m_elapsedTime = 0.0f; // 経過時間
public:
    ExplosionEffect() = delete;
    explicit ExplosionEffect(GameObject* owner)
        : Component(owner)
    {
    }
    void Update() override;
};
