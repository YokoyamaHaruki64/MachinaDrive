#pragma once

#include "System/Core/Script.h"

class ComboUI : public Script
{
    GM_SCRIPT(ComboUI, "Combo UI");
    USE_FIELDS
private:
    GameObjectReference m_comboValue;
    UITransform* m_comboTransform = nullptr;
    UIText* m_comboText = nullptr;
    Vector2 m_baseScale{ 1.0f, 1.0f };
    int m_comboCount = 0;

    float m_boundTime = 0.5f;
    float m_timer = 0.0f;

    float m_boundScale = 0.25f;

    Curve<float> m_boundCurve;

public:
    void OnStart() override;
    void Update() override;
    void AddCombo();
    void SetCount(int count);
    void ResetCombo();
};
