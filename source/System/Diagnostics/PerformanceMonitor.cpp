#include "pch.h"

#include <algorithm>

#include "imgui.h"

#include "System/Diagnostics/PerformanceMonitor.h"

PerformanceMonitor::SampleHistory PerformanceMonitor::s_frameTime;
PerformanceMonitor::SampleHistory PerformanceMonitor::s_updateTime;
PerformanceMonitor::SampleHistory PerformanceMonitor::s_lateUpdateTime;
PerformanceMonitor::SampleHistory PerformanceMonitor::s_renderTime;

PerformanceMonitor::Clock::time_point PerformanceMonitor::s_updateStart;
PerformanceMonitor::Clock::time_point PerformanceMonitor::s_lateUpdateStart;
PerformanceMonitor::Clock::time_point PerformanceMonitor::s_postLateUpdateStart;
PerformanceMonitor::Clock::time_point PerformanceMonitor::s_renderStart;
float PerformanceMonitor::s_updateBeforeLateMilliseconds = 0.0f;

size_t PerformanceMonitor::s_activeObjectCount = 0;
size_t PerformanceMonitor::s_pendingObjectCount = 0;


float PerformanceMonitor::GetElapsedMilliseconds(
    const Clock::time_point& start,
    const Clock::time_point& end)
{
    return std::chrono::duration<float, std::milli>(end - start).count();
}


void PerformanceMonitor::DrawTimeRow(const char* label, const SampleHistory& history)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text("%.3f ms", history.GetCurrent());
    ImGui::TableSetColumnIndex(2);
    ImGui::Text("%.3f ms", history.GetAverage());
    ImGui::TableSetColumnIndex(3);
    ImGui::Text("%.3f ms", history.GetMax());
}


void PerformanceMonitor::RecordFrameTime(float seconds)
{
    s_frameTime.Add(seconds * 1000.0f);
}


void PerformanceMonitor::BeginUpdate()
{
    s_updateStart = Clock::now();
}


void PerformanceMonitor::BeginLateUpdate()
{
    s_lateUpdateStart = Clock::now();
    s_updateBeforeLateMilliseconds = GetElapsedMilliseconds(s_updateStart, s_lateUpdateStart);
}


void PerformanceMonitor::EndLateUpdate()
{
    s_postLateUpdateStart = Clock::now();
    s_lateUpdateTime.Add(GetElapsedMilliseconds(s_lateUpdateStart, s_postLateUpdateStart));
}


void PerformanceMonitor::EndUpdate()
{
    const Clock::time_point updateEnd = Clock::now();
    const float postLateUpdateMilliseconds = GetElapsedMilliseconds(s_postLateUpdateStart, updateEnd);
    s_updateTime.Add(s_updateBeforeLateMilliseconds + postLateUpdateMilliseconds);
}


void PerformanceMonitor::BeginRender()
{
    s_renderStart = Clock::now();
}


void PerformanceMonitor::EndRender()
{
    s_renderTime.Add(GetElapsedMilliseconds(s_renderStart, Clock::now()));
}


void PerformanceMonitor::SetObjectCounts(size_t activeCount, size_t pendingCount)
{
    s_activeObjectCount = activeCount;
    s_pendingObjectCount = pendingCount;
}


void PerformanceMonitor::DrawImGuiTab()
{
    if (!ImGui::BeginTabItem("Performance"))
    {
        return;
    }

    const float averageFrameTime = s_frameTime.GetAverage();
    const float fps = averageFrameTime > 0.0f ? 1000.0f / averageFrameTime : 0.0f;
    ImGui::Text("FPS: %.1f", fps);

    constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if (ImGui::BeginTable("PerformanceTimes", 4, tableFlags))
    {
        ImGui::TableSetupColumn("Metric");
        ImGui::TableSetupColumn("Current");
        ImGui::TableSetupColumn("Average");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        DrawTimeRow("Frame Time", s_frameTime);
        DrawTimeRow("Update Time", s_updateTime);
        DrawTimeRow("LateUpdate Time", s_lateUpdateTime);
        DrawTimeRow("Render Time", s_renderTime);

        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::Text("Objects: Active %zu / Pending %zu", s_activeObjectCount, s_pendingObjectCount);

    ImGui::EndTabItem();
}


void PerformanceMonitor::SampleHistory::Add(float milliseconds)
{
    current = milliseconds;
    samples[nextIndex] = milliseconds;
    nextIndex = (nextIndex + 1) % samples.size();
    count = std::min(count + 1, samples.size());
}


float PerformanceMonitor::SampleHistory::GetCurrent() const
{
    return current;
}


float PerformanceMonitor::SampleHistory::GetAverage() const
{
    if (count == 0)
    {
        return 0.0f;
    }

    float total = 0.0f;
    for (size_t i = 0; i < count; ++i)
    {
        total += samples[i];
    }

    return total / static_cast<float>(count);
}


float PerformanceMonitor::SampleHistory::GetMax() const
{
    if (count == 0)
    {
        return 0.0f;
    }

    return *std::max_element(samples.begin(), samples.begin() + count);
}
