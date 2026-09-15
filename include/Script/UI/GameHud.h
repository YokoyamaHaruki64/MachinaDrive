#pragma once

#include <vector>
#include <unordered_set>

#include "System/Core/Script.h"

// ゲームロジックから表示状態を受け取る窓口。参照先はSceneで設定する。
class GameHud final : public Script
{
    GM_SCRIPT(GameHud, "Game HUD")
    USE_FIELDS

    GameObjectReference m_healthBar;
    GameObjectReference m_timerText;
    GameObjectReference m_waveNumber;
    GameObjectReference m_enemyNumber;
    GameObjectReference m_comboUIObject;
    GameObjectReference m_shardsNumber;
    GameObjectReference m_healthRoot;
    GameObjectReference m_waveRoot;
    GameObjectReference m_comboRoot;
    GameObjectReference m_driveRoot;
    GameObjectReference m_shardsRoot;
    GameObjectReference m_driveGauge;
    GameObjectReference m_driveChargingText;
    GameObjectReference m_driveReadyText;

    class ComboUI* m_comboUI = nullptr;
    class WaveStartUI* m_waveStartUI = nullptr;
    class TimeRecoveryUI* m_timeRecoveryUI = nullptr;
    struct FadeTarget
    {
        UIImage* image = nullptr;
        UIText* text = nullptr;
        Color color{};
    };

    std::vector<FadeTarget> m_fadeTargets;
    float m_fadeDuration{};
    float m_fadeElapsedTime{};
    bool m_isFadingOut{};
    bool m_visible = true;
    bool m_comboVisible = true;
    bool m_driveReady = true;
    int m_comboCount = 0;

    void ApplyVisibility();
    void ApplyDriveState();
    void CollectFadeTargets(GameObject* object, std::unordered_set<GameObject*>& visited);
    void ApplyFade(float alpha);

public:
    void OnStart() override;
    void Update() override;
    void SetHealth(float current, float maximum);
    // 被ダメージ量を反映し、FillBarのChip表示を維持する。
    void ApplyHealthDamage(float damage);
    void SetWave(int wave);
    void SetEnemyCount(int count);
    void SetCombo(int count);
    int GetComboCount() const { return m_comboCount; }
    void AddCombo();
    void ResetCombo();
    void SetShards(int count);
    void SetVisible(bool visible);
    // Unscaled時間でHUD全体を透明化し、完了時に非アクティブ化する。
    void FadeOut(float duration);
    void SetComboVisible(bool visible);
    // 0.0（空）から1.0（満タン）までをGaugeInnerへ渡す。
    void SetDriveGauge(float progress);
    // ReadyとChargingの表示を排他的に切り替える。
    void SetDriveReady(bool ready);
    void SetDriveCharging(bool charging) { SetDriveReady(!charging); }
    // Wave待機中の0.0（空）から1.0（満タン）までの進捗を表示へ渡す。
    void SetWaveStartProgress(float progress);
    // Wave開始表示へ切り替える。
    void ShowWaveBegin();
    // Wave開始表示の進捗を表示へ渡す。
    void SetWaveBeginProgress(float progress);
    // 実際に回復した時間を、上へ移動しながらフェードアウトさせる。
    void ShowTimeRecovery(float amount);
};
