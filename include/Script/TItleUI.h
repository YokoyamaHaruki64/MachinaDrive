#pragma once

#include "System/Core/Script.h"

class TitleUI : public Script
{
    REGISTER_COMPONENT(TitleUI, "TitleUI");
private:
    Vector2 m_originalOffset = Vector2::Zero();
    float m_amplitude = 15.0f; // 振幅
    float m_frequency = 0.5f; // 周波数
    float m_elapsedTime = 0.0f; // 経過時間
public:
    TitleUI() = delete;
    explicit TitleUI(GameObject* owner)
        : Script(owner)
    {
    }
    void OnStart() override;
    void Update() override;
};
