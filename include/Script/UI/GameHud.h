#pragma once

#include "System/Core/Script.h"

// ゲームロジックから表示状態を受け取る窓口。参照先はSceneで設定する。
class GameHud final : public Script
{
    GM_SCRIPT(GameHud, "Game HUD")
    USE_FIELDS

    GameObjectReference m_healthBar;
    GameObjectReference m_waveNumber;
    GameObjectReference m_enemyNumber;
    GameObjectReference m_comboNumber;
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
    bool m_visible = true;
    bool m_comboVisible = true;
    bool m_driveReady = true;
    int m_comboCount = 0;

    void ApplyVisibility();
    void ApplyDriveState();

public:
    void OnStart() override;
    void SetHealth(float current, float maximum);
    // 被ダメージ量を反映し、FillBarのChip表示を維持する。
    void ApplyHealthDamage(float damage);
    void SetWave(int wave);
    void SetEnemyCount(int count);
    void SetCombo(int count);
    void AddCombo();
    void ResetCombo();
    void SetShards(int count);
    void SetVisible(bool visible);
    void SetComboVisible(bool visible);
    // 0.0（空）から1.0（満タン）までをGaugeInnerへ渡す。
    void SetDriveGauge(float progress);
    // ReadyとChargingの表示を排他的に切り替える。
    void SetDriveReady(bool ready);
    void SetDriveCharging(bool charging) { SetDriveReady(!charging); }
};
