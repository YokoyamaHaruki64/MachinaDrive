#include "pch.h"

#include "System/GM_Time.h"

float Time::m_timeScale = 1.0f;
float Time::m_deltaTime = 0.0f;
float Time::m_unscaledDeltaTime = 0.0f;
float Time::m_totalTime = 0.0f;
float Time::m_unscaledTotalTime = 0.0f;

void Time::Update(float deltaTime)
{
    m_unscaledDeltaTime = deltaTime;
    m_deltaTime = deltaTime * m_timeScale;
    m_unscaledTotalTime += m_unscaledDeltaTime;
    m_totalTime += m_deltaTime;
}
