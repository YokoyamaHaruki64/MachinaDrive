#include "pch.h"
#include "Script/UI/GameHud.h"

#include "Script/UI/HudNumber.h"
#include "Script/ComboUI.h"
#include "Script/FillBar.h"
#include "Component/UIImage.h"
#include "System/Core/GameObject.h"
#include "System/Core/manager.h"
#include "System/Editor/FieldRegistry.h"

namespace
{
    void SetNumber(const GameObjectReference& target, int value)
    {
        if (target)
        {
            if (auto* number = target->GetComponent<HudNumber>())
            {
                number->SetValue(value);
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
    GM_FIELD(m_waveNumber);
    GM_FIELD(m_enemyNumber);
    GM_FIELD(m_comboNumber);
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

    // 現時点でシーンから確定できる初期値だけを表示へ反映する。
    SetWave(1);
    SetEnemyCount(static_cast<int>(Manager::GetObjectsByTag("Enemy").size()));
    ResetCombo();
    ApplyVisibility();
    ApplyDriveState();
}

void GameHud::SetHealth(float current, float maximum)
{
    if (!m_healthBar || !std::isfinite(current) || !std::isfinite(maximum))
    {
        return;
    }

    if (auto* bar = m_healthBar->GetComponent<FillBar>())
    {
        bar->SetMax((std::max)(0.0f, maximum));
        bar->SetCurrent(current);
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
    SetNumber(m_waveNumber, wave);
}

void GameHud::SetEnemyCount(int count)
{
    SetNumber(m_enemyNumber, count);
}

void GameHud::SetCombo(int count)
{
    m_comboCount = (std::max)(0, count);
    SetNumber(m_comboNumber, m_comboCount);
    SetComboVisible(m_comboCount > 0);
}

void GameHud::AddCombo()
{
    if (m_comboUI)
    {
        m_comboUI->AddCombo();
    }

    SetCombo(m_comboCount + 1);
}

void GameHud::ResetCombo()
{
    if (m_comboUI)
    {
        m_comboUI->ResetCombo();
    }

    SetCombo(0);
}

void GameHud::SetShards(int count)
{
    SetNumber(m_shardsNumber, count);
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
