#include "pch.h"
#include "Script/UI/GameHud.h"

#include "Script/ComboUI.h"
#include "Script/FillBar.h"
#include "Script/UI/TimeRecoveryUI.h"
#include "Script/UI/WaveStartUI.h"
#include "Component/UIImage.h"
#include "Component/UIText.h"
#include "System/Core/GameObject.h"
#include "System/Core/manager.h"
#include "System/Editor/FieldRegistry.h"

#include <cmath>
#include <cstdio>

namespace
{
    void SetText(const GameObjectReference& target, const char* format, int value)
    {
        if (target)
        {
            if (auto* text = target->GetComponent<UIText>())
            {
                char buffer[32]{};
                std::snprintf(buffer, sizeof(buffer), format, value);
                text->SetText(buffer);
            }
        }
    }

    void SetTreeVisible(GameObject* object, bool visible)
    {
        if (!object || object->IsDestroyed())
        {
            return;
        }

        // このエンジンでは親のActiveが子へ伝播しないため、表示階層全体へ適用する。
        object->SetActive(visible);
        for (auto* child : object->GetTransformBase()->GetChildren())
        {
            SetTreeVisible(child->GetOwner(), visible);
        }
    }
}

FIELDS_BEGIN(GameHud)
    GM_FIELD(m_healthBar);
    GM_FIELD(m_timerText);
    GM_FIELD(m_waveNumber);
    GM_FIELD(m_enemyNumber);
    GM_FIELD(m_comboUIObject);
    GM_FIELD(m_shardsNumber);
    GM_FIELD(m_healthRoot);
    GM_FIELD(m_waveRoot);
    GM_FIELD(m_comboRoot);
    GM_FIELD(m_driveRoot);
    GM_FIELD(m_shardsRoot);
    GM_FIELD(m_driveGauge);
    GM_FIELD(m_driveChargingText);
    GM_FIELD(m_driveReadyText);
    GM_FIELD(m_visible);
    GM_FIELD(m_comboVisible);
FIELDS_END()

void GameHud::OnStart()
{
    m_comboUI = m_comboUIObject.Get() ? m_comboUIObject->GetComponent<ComboUI>() : nullptr;
    m_waveStartUI = m_waveRoot.Get() ? m_waveRoot->GetComponent<WaveStartUI>() : nullptr;
    m_timeRecoveryUI = m_healthRoot.Get() ? m_healthRoot->GetComponent<TimeRecoveryUI>() : nullptr;

    // 現時点でシーンから確定できる初期値だけを表示へ反映する。
    SetWave(1);
    SetEnemyCount(static_cast<int>(Manager::GetObjectsByTag("Enemy").size()));
    ResetCombo();
    ApplyVisibility();
}

void GameHud::SetHealth(float current, float maximum)
{
    if (!std::isfinite(current) || !std::isfinite(maximum))
    {
        return;
    }

    const float clampedMaximum = (std::max)(0.0f, maximum);
    const float clampedCurrent = Math::Clamp(current, 0.0f, clampedMaximum);
    if (m_healthBar)
    {
        if (auto* bar = m_healthBar->GetComponent<FillBar>())
        {
            bar->SetMax(clampedMaximum);
            bar->SetCurrent(clampedCurrent);
        }
    }

    if (m_timerText)
    {
        if (auto* text = m_timerText->GetComponent<UIText>())
        {
            char buffer[16]{};
            std::snprintf(buffer, sizeof(buffer), "%06.2f", Math::Min(clampedCurrent, 999.99f));
            text->SetText(buffer);
        }
    }
}

void GameHud::ApplyHealthDamage(float damage)
{
    if (!m_healthBar || !std::isfinite(damage) || damage <= 0.0f)
    {
        return;
    }

    if (auto* bar = m_healthBar->GetComponent<FillBar>())
    {
        bar->AddCurrent(-damage);
    }
}

void GameHud::SetWave(int wave)
{
    SetText(m_waveNumber, "%02d", (std::max)(0, wave));
}

void GameHud::SetEnemyCount(int count)
{
    SetText(m_enemyNumber, "%02d", (std::max)(0, count));
}

void GameHud::SetCombo(int count)
{
    m_comboCount = (std::max)(0, count);
    SetComboVisible(m_comboCount > 0);
    if (m_comboUI)
    {
        m_comboUI->SetCount(m_comboCount);
    }
}

void GameHud::AddCombo()
{
    SetCombo(m_comboCount + 1);
}

void GameHud::ResetCombo()
{
    SetCombo(0);
}

void GameHud::SetShards(int count)
{
    (void)count;
}

void GameHud::SetVisible(bool visible)
{
    m_visible = visible;
    ApplyVisibility();
}

void GameHud::SetComboVisible(bool visible)
{
    m_comboVisible = visible;
    ApplyVisibility();
}

void GameHud::ApplyVisibility()
{
    SetTreeVisible(m_healthRoot.Get(), m_visible);
    SetTreeVisible(m_waveRoot.Get(), m_visible);
    SetTreeVisible(m_comboRoot.Get(), m_visible && m_comboVisible);
    SetTreeVisible(m_driveRoot.Get(), m_visible);
    SetTreeVisible(m_shardsRoot.Get(), m_visible);
    ApplyDriveState();
    if (m_waveStartUI)
    {
        m_waveStartUI->SetVisible(m_visible);
    }
    if (m_timeRecoveryUI)
    {
        m_timeRecoveryUI->SetVisible(m_visible);
    }
}

void GameHud::ApplyDriveState()
{
    if (m_driveReadyText)
    {
        m_driveReadyText->SetActive(m_visible && m_driveReady);
    }
    if (m_driveChargingText)
    {
        m_driveChargingText->SetActive(m_visible && !m_driveReady);
    }
}

void GameHud::SetDriveGauge(float progress)
{
    if (!m_driveGauge || !std::isfinite(progress))
    {
        return;
    }

    const float clampedProgress = (std::max)(0.0f, (std::min)(1.0f, progress));
    if (auto* image = m_driveGauge->GetComponent<UIImage>())
    {
        image->GetMaterialPropertyOverride().SetFloat("Progress", clampedProgress);
    }
}

void GameHud::SetDriveReady(bool ready)
{
    m_driveReady = ready;
    ApplyDriveState();
}

void GameHud::SetWaveStartProgress(float progress)
{
    if (m_waveStartUI)
    {
        m_waveStartUI->SetProgress(progress);
    }
}

void GameHud::ShowWaveBegin()
{
    if (m_waveStartUI)
    {
        m_waveStartUI->ShowBegin();
    }
}

void GameHud::SetWaveBeginProgress(float progress)
{
    if (m_waveStartUI)
    {
        m_waveStartUI->SetBeginProgress(progress);
    }
}

void GameHud::ShowTimeRecovery(float amount)
{
    if (m_timeRecoveryUI)
    {
        m_timeRecoveryUI->ShowRecovery(amount);
    }
}
