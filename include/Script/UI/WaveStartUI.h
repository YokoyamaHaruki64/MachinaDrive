#pragma once

#include "System/Core/Script.h"

class FillBar;
class UIFade;
class WaveDirector;

// HUD_Wave配下の待機ProgressBarとWave開始表示を管理する。
class WaveStartUI final : public Script
{
    GM_SCRIPT(WaveStartUI, "Wave Start UI")
    USE_FIELDS

    enum class State
    {
        Hidden,
        Countdown,
        Begin,
    };

    GameObjectReference m_frame;
    GameObjectReference m_beginText;

    GameObjectReference m_progressTextObject;

    float m_onDuration = 0.25f;
    float m_offDuration = 0.25f;

    FillBar* m_progressBar = nullptr;
    UIFade* m_beginFade = nullptr;
    WaveDirector* m_waveDirector = nullptr;
    float m_progress = 0.0f;
    State m_state = State::Hidden;
    bool m_visible = true;

    void ResolveTargets();
    void ApplyProgress();
    void ApplyVisibility();

public:
    void OnStart() override;

    void SetVisible(bool visible);
    void SetProgress(float progress);
    float GetProgress() const { return m_progress; }
    void ShowBegin();
    void SetBeginProgress(float progress);
};
