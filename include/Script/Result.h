#pragma once

#include "System/Core/Script.h"

class GrayScalePostProcess;
class GameHud;

// タイムアップ後の停止演出を管理し、完了後にResult UIへ遷移可能な状態を通知する。
class Result final : public Script
{
    GM_SCRIPT(Result, "Result")
    USE_FIELDS

    inline static Result* s_instance{};
    inline static bool s_isResult{};

    float m_duration = 1.0f;
    float m_elapsedTime{};
    bool m_isTransitioning{};
    GrayScalePostProcess* m_grayScalePostProcess{};
    GameHud* m_gameHud{};

    void ResolveGrayScalePostProcess();
    void ResolveGameHud();
    void Begin();

public:
    void OnStart() override;
    void OnDestroy() override;
    void Update() override;

    // WaveDirectorなどから呼び、タイムアップ演出を開始する。
    static bool BeginResult();
    // TimeScale停止とグレースケール演出が完了し、Result UIを表示できる状態か。
    static bool IsResult() { return s_isResult; }
};
