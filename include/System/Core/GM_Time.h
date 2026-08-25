#pragma once

class Time
{
    static float m_timeScale;

    static float m_deltaTime;
    static float m_unscaledDeltaTime;
    static float m_totalTime;
    static float m_unscaledTotalTime;

public:
    static float GetTimeScale() { return m_timeScale; }
    static void SetTimeScale(float timeScale) { m_timeScale = timeScale; }
    static float GetDeltaTime() { return m_deltaTime; }
    static float GetUnscaledDeltaTime() { return m_unscaledDeltaTime; }
    static float GetTotalTime() { return m_totalTime; }
    static float GetUnscaledTotalTime() { return m_unscaledTotalTime; }
    static void Update(float deltaTime);
};
