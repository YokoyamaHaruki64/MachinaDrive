#include "pch.h"

#include "Script/Result.h"

#include "Script/GrayScalePostProcess.h"
#include "Script/UI/GameHud.h"
#include "Script/WaveDirector.h"
#include "System/Core/GM_Time.h"
#include "System/Core/manager.h"
#include "System/Diagnostics/Logger.h"
#include "System/Editor/FieldRegistry.h"

FIELDS_BEGIN(Result)
    GM_FIELD(m_duration);
FIELDS_END()

void Result::OnStart()
{
    s_instance = this;
    m_duration = Math::Max(m_duration, 0.0f);
    m_elapsedTime = 0.0f;
    m_survivalTime = 0.0f;
    m_state = ResultState::Playing;

    m_maxCombo = 0;
    m_wave = 0;
    m_defeatedEnemyCount = 0;

    ResolveGrayScalePostProcess();
    ResolveGameHud();
    ResolveWaveDirector();
    if (m_grayScalePostProcess)
    {
        m_grayScalePostProcess->SetIntensity(0.0f);
    }
}

void Result::OnDestroy()
{
    if (s_instance != this)
    {
        return;
    }

    // 次のSceneへ移る際、停止状態を持ち越さない。
    Time::SetTimeScale(1.0f);
    s_instance = nullptr;
}

bool Result::BeginResult()
{
    if (!s_instance)
    {
        Logger::Error("Result: Result ScriptがSceneに存在しません。");
        return false;
    }

    if (s_instance->m_state != ResultState::Playing)
    {
        return false;
    }

    s_instance->Begin();
    return true;
}

void Result::AddDefeatedEnemy()
{
    if (!s_instance || s_instance->m_state != ResultState::Playing)
    {
        return;
    }

    ++s_instance->m_defeatedEnemyCount;
}

void Result::Update()
{
    switch (m_state)
    {
    case ResultState::Playing:
        CollectGameStats();
        break;

    case ResultState::Transition:
        UpdateTransition();
        break;

    case ResultState::Completed:
        break;
    }
}

void Result::UpdateTransition()
{
    m_elapsedTime += Time::GetUnscaledDeltaTime();
    const float progress = m_duration > 0.0f
        ? Math::Clamp(m_elapsedTime / m_duration, 0.0f, 1.0f)
        : 1.0f;

    Time::SetTimeScale(1.0f - progress);
    if (m_grayScalePostProcess)
    {
        m_grayScalePostProcess->SetIntensity(progress);
    }

    if (progress >= 1.0f)
    {
        Time::SetTimeScale(0.0f);
        m_state = ResultState::Completed;
    }
}

void Result::ResolveGrayScalePostProcess()
{
    if (m_grayScalePostProcess)
    {
        return;
    }

    if (GameObject* const postProcessObject = Manager::GetFirstObjectByComponent<GrayScalePostProcess>())
    {
        m_grayScalePostProcess = postProcessObject->GetComponent<GrayScalePostProcess>();
    }
}

void Result::Begin()
{
    ResolveGrayScalePostProcess();
    ResolveGameHud();
    if (!m_grayScalePostProcess)
    {
        Logger::Warning("Result: GrayScalePostProcessが見つかりません。グレースケール演出を省略します。");
    }

    m_elapsedTime = 0.0f;
    m_state = ResultState::Transition;
    if (m_gameHud)
    {
        m_gameHud->FadeOut(m_duration);
    }
}

void Result::ResolveGameHud()
{
    if (m_gameHud)
    {
        return;
    }

    if (GameObject* const gameHudObject = Manager::GetFirstObjectByComponent<GameHud>())
    {
        m_gameHud = gameHudObject->GetComponent<GameHud>();
    }
}

void Result::ResolveWaveDirector()
{
    if (m_waveDirector)
    {
        return;
    }

    if (GameObject* const waveDirectorObject = Manager::GetFirstObjectByComponent<WaveDirector>())
    {
        m_waveDirector = waveDirectorObject->GetComponent<WaveDirector>();
    }
}

void Result::CollectGameStats()
{
    m_survivalTime += Time::GetDeltaTime();

    ResolveGameHud();
    if (m_gameHud)
    {
        m_maxCombo = Math::Max(m_maxCombo, m_gameHud->GetComboCount());
    }

    ResolveWaveDirector();
    if (m_waveDirector)
    {
        m_wave = m_waveDirector->GetWave();
    }
}
