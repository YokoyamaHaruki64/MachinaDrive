#include "pch.h"

#include "Script/Result.h"

#include "Script/GrayScalePostProcess.h"
#include "Script/UI/GameHud.h"
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
    s_isResult = false;
    m_duration = Math::Max(m_duration, 0.0f);
    m_elapsedTime = 0.0f;
    m_isTransitioning = false;

    ResolveGrayScalePostProcess();
    ResolveGameHud();
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
    s_isResult = false;
}

bool Result::BeginResult()
{
    if (!s_instance)
    {
        Logger::Error("Result: Result ScriptがSceneに存在しません。");
        return false;
    }

    if (s_instance->m_isTransitioning || s_isResult)
    {
        return false;
    }

    s_instance->Begin();
    return true;
}

void Result::Update()
{
    if (!m_isTransitioning)
    {
        return;
    }

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
        m_isTransitioning = false;
        s_isResult = true;
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
    m_isTransitioning = true;
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
