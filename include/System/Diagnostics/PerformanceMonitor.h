#pragma once

#include <array>
#include <chrono>
#include <cstddef>

// メインループのCPU処理時間とオブジェクト数を収集・表示する。
// Update時間にはOnStartや破棄処理を含め、LateUpdate時間だけを分離する。
class PerformanceMonitor final
{
public:
    static constexpr size_t SampleCount = 60;

    static void RecordFrameTime(float seconds);

    static void BeginUpdate();
    static void BeginLateUpdate();
    static void EndLateUpdate();
    static void EndUpdate();

    static void BeginRender();
    static void EndRender();

    static void SetObjectCounts(size_t activeCount, size_t pendingCount);
    static void DrawImGui();

private:
    using Clock = std::chrono::steady_clock;

    struct SampleHistory
    {
        void Add(float milliseconds);
        float GetCurrent() const;
        float GetAverage() const;
        float GetMax() const;

        std::array<float, SampleCount> samples{};
        size_t count = 0;
        size_t nextIndex = 0;
        float current = 0.0f;
    };

    static float GetElapsedMilliseconds(
        const Clock::time_point& start,
        const Clock::time_point& end);
    static void DrawTimeRow(const char* label, const SampleHistory& history);

    static SampleHistory s_frameTime;
    static SampleHistory s_updateTime;
    static SampleHistory s_lateUpdateTime;
    static SampleHistory s_renderTime;

    static Clock::time_point s_updateStart;
    static Clock::time_point s_lateUpdateStart;
    static Clock::time_point s_postLateUpdateStart;
    static Clock::time_point s_renderStart;
    static float s_updateBeforeLateMilliseconds;

    static size_t s_activeObjectCount;
    static size_t s_pendingObjectCount;
};
