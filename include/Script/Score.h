#pragma once

#include "System/Script.h"
#include "System/GameObjectReference.h"

class GameObject;

class Score : public Script
{
    static constexpr int MAX_DIGITS = 4;        // スコア表示の最大桁数
    int m_score = 0;
    std::vector<GameObjectReference> m_scoreObjects;    // スコア表示用のGameObjectのリスト

public:
    Score() = delete;
    explicit Score(GameObject* owner) : Script(owner) {}
    virtual ~Score() = default;
    void AddScore(int amount) { m_score += amount; }
    int GetScore() const { return m_score; }
    void OnStart() override;
    void Update() override;
};
