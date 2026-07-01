#pragma once

#include "System/component.h"

class TitleUI : public Component
{
private:
    Vector3 m_originalPosition = Vector3::Zero();
    float m_amplitude = 15.0f; // 振幅
    float m_frequency = 0.5f; // 周波数
    float m_elapsedTime = 0.0f; // 経過時間
public:
    TitleUI() = delete;
    explicit TitleUI(GameObject* owner)
        : Component(owner)
    {
    }
    void OnStart() override;
    void Update() override;
};
