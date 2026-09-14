#pragma once

#include "System/Core/Script.h"

class GrayScalePostProcess;
class GameHud;
class WaveDirector;

// タイムアップ後の停止演出を管理し、完了後にResult UIへ遷移可能な状態を通知する。
class Result final : public Script
{
public:
    enum class ResultState
    {
        Playing,
        Transition,
        Completed,
    };

private:
    GM_SCRIPT(Result, "Result")
    USE_FIELDS

    inline static Result* s_instance{};

    float m_duration = 1.0f;
    float m_elapsedTime{};
    float m_survivalTime{};
    ResultState m_state = ResultState::Playing;
    int m_maxCombo{};
    int m_wave{};
    int m_defeatedEnemyCount{};
    GrayScalePostProcess* m_grayScalePostProcess{};
    GameHud* m_gameHud{};
    WaveDirector* m_waveDirector{};

    void ResolveGrayScalePostProcess();
    void ResolveGameHud();
    void ResolveWaveDirector();
    void CollectGameStats();
    void UpdateTransition();
    void Begin();

public:
    void OnStart() override;
    void OnDestroy() override;
    void Update() override;

    // WaveDirectorなどから呼び、タイムアップ演出を開始する。
    static bool BeginResult();
    static ResultState GetResultState()
    {
        return s_instance ? s_instance->m_state : ResultState::Playing;
    }
    // TimeScale停止とグレースケール演出が完了し、Result UIを表示できる状態か。
    static bool IsResult() { return GetResultState() == ResultState::Completed; }
    static float GetSurvivalTime() { return s_instance ? s_instance->m_survivalTime : 0.0f; }
    static int GetMaxCombo() { return s_instance ? s_instance->m_maxCombo : 0; }
    static int GetWave() { return s_instance ? s_instance->m_wave : 0; }
    static int GetDefeatedEnemyCount() { return s_instance ? s_instance->m_defeatedEnemyCount : 0; }
    // 敵の死亡確定時に呼び、Scene破棄などを撃破数に含めない。
    static void AddDefeatedEnemy();
};
