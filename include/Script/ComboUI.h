#pragma once

#include "System/Core/Script.h"

class ComboUI : public Script
{
    GM_SCRIPT(ComboUI, "Combo UI");
    USE_FIELDS
private:
    GameObjectReference m_comboValue;
    GameObjectReference m_comboLabel;
    UIText* m_comboText = nullptr;
    UIText* m_comboLabelText = nullptr;
    UITransform* m_comboValueTransform = nullptr;
    UITransform* m_comboLabelTransform = nullptr;
    float m_originalFontSize{ 32.0f };
    float m_originalLabelFontSize{ 32.0f };
    Vector2 m_originalValueOffset{};
    Vector2 m_originalLabelOffset{};
    int m_comboCount = 0;

    float m_boundTime = 0.5f;
    float m_timer = 0.0f;

    float m_boundScale = 0.25f;
    float m_gap = 36.0f;

    Curve<float> m_boundCurve;

    void ApplyScale(float scale);

public:
    void OnStart() override;
    void Update() override;
    void AddCombo();
    void SetCount(int count);
    void ResetCombo();
};
