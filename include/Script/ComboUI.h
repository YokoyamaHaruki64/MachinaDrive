#pragma once

#include "System/Core/Script.h"

class ComboUI : public Script
{
    GM_SCRIPT(ComboUI, "Combo UI");
    USE_FIELDS
private:
    UITransform* m_comboTextTransform = nullptr;
    UIText* m_comboText = nullptr;
    int m_comboCount = 0;

    float m_boundTime = 0.5f;
    float m_timer = 0.0f;

    float m_boundScale = 75.0f;

    Curve<float> m_boundCurve;

public:
    void OnStart() override;
    void Update() override;
    void AddCombo();
    void ResetCombo();
};
