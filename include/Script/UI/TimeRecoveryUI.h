#pragma once

#include "System/Core/Script.h"

class UIText;
class UIFade;
class UIOffset;

// HUD_Timer配下の時間回復テキストを上方向へ流して消す。
class TimeRecoveryUI final : public Script
{
    GM_SCRIPT(TimeRecoveryUI, "Time Recovery UI")
    USE_FIELDS

    GameObjectReference m_recoveryText;
    float m_duration = 0.75f;

    UIText* m_text = nullptr;
    UIFade* m_fade = nullptr;
    UIOffset* m_offset = nullptr;
    float m_progress = 1.0f;
    int m_roundedAmount = 0;
    bool m_visible = true;

    void ResolveTargets();
    void ApplyContent();
    void ApplyProgress();
    void ApplyVisibility();

public:
    void OnStart() override;
    void Update() override;

    void SetVisible(bool visible);
    void ShowRecovery(float amount);
};
