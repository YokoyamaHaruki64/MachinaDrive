#pragma once

#include "System/Core/Script.h"

class Player;

// プレイヤーの発射操作からBasicEnemy撃破までのチュートリアルを進行する。
class TutorialDirector final : public Script
{
    GM_SCRIPT(TutorialDirector, "Tutorial Director")
    USE_FIELDS

    enum class TutorialState
    {
        WaitingForShot,
        WaitingForPlayerStop,
        WaitingForEnemyDefeat,
        Complete,
    };

    GameObjectReference m_player;
    GameObjectReference m_instructionText;
    PrefabReference m_basicEnemyPrefab{ "Assets/Prefabs/Enemy.prefab" };
    Vector3 m_enemySpawnPosition{ 0.0f, 0.0f, 15.0f };
    float m_completeWaitDuration = 1.0f;

    Player* m_playerScript = nullptr;
    Rigidbody* m_playerRigidbody = nullptr;
    UIText* m_instructionTextComponent = nullptr;
    GameObjectReference m_enemy;
    Vector3 m_centerPosition = Vector3::Zero();
    float m_stateTimer = 0.0f;
    TutorialState m_state = TutorialState::WaitingForShot;

    void SetInstruction(const char* text);
    void ReturnToCenter();
    void SpawnBasicEnemy();
    void CompleteTutorial();

public:
    void OnStart() override;
    void Update() override;
};
