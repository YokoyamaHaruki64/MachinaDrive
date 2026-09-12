#pragma once

#include "System/Core/PrefabReference.h"
#include "System/Core/Script.h"

class GameHud;

// 時間制のWave進行と、Field内への敵出現を一元管理する。
class WaveDirector final : public Script
{
    GM_SCRIPT(WaveDirector, "Wave Director")
    USE_FIELDS

    PrefabReference m_basicEnemyPrefab{ "Assets/Prefabs/Enemy.prefab" };
    PrefabReference m_movingEnemyPrefab{ "Assets/Prefabs/MovingEnemy.prefab" };
    PrefabReference m_heavyEnemyPrefab{ "Assets/Prefabs/HeavyEnemy.prefab" };

    GameObjectReference m_player;

    enum class WaveState
    {
        Combat,
        WaitingForPlayerIdle,
        Intermission,
    };

    float m_maxTime = 60.0f;
    float m_startTime = 60.0f;
    float m_timeDrainPerSecond = 1.0f;
    float m_baseClearTimeBonus = 8.0f;
    float m_clearTimeGrowth = 2.0f;
    float m_maxClearTimeBonus = 20.0f;
    float m_waveStartDelay = 2.0f;

    int m_baseEnemyCount = 3;
    float m_enemyCountGrowth = 1.5f;
    float m_enemyCountExponent = 1.25f;
    int m_maxEnemyCount = 16;
    float m_movingStart = 0.15f;
    float m_movingGrowth = 0.02f;
    float m_movingMax = 0.40f;
    int m_heavyStartWave = 3;
    float m_heavyGrowth = 0.03f;
    float m_heavyMax = 0.35f;
    float m_minPlayerDistance = 12.0f;
    int m_spawnAttemptCount = 16;
    float m_spawnHeight = 0.0f;

    GameHud* m_gameHud = nullptr;
    int m_wave = 0;
    float m_remainingTime = 0.0f;
    float m_minX = 0.0f;
    float m_maxX = 0.0f;
    float m_minZ = 0.0f;
    float m_maxZ = 0.0f;
    float m_stateTimer = 0.0f;
    WaveState m_waveState = WaveState::Combat;
    bool m_hasSpawnArea = false;
    bool m_hasFinished = false;

    bool ResolveSpawnArea();
    void DestroyPlacedEnemies();
    void StartNextWave();
    void BeginWaveClear();
    bool IsPlayerIdle() const;
    bool SpawnWaveEnemies();
    Vector3 CreateSpawnPosition() const;
    const PrefabReference& SelectEnemyPrefab() const;
    int CalculateEnemyCount() const;
    float CalculateClearTimeBonus() const;
    void AddTime(float amount);
    void UpdateHud() const;
    void FinishGame();

public:
    void OnStart() override;
    void Update() override;

    int GetWave() const { return m_wave; }
    float GetRemainingTime() const { return m_remainingTime; }
    void AddRemainingTime(float amount) { AddTime(amount); }
};
