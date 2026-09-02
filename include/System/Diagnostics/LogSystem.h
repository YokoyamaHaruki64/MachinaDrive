#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <string>

#include "System/Diagnostics/Logger.h"

class LogFileWorker;

// フレーム終端でログを取り出し、各出力先へ配送する。
class LogSystem final
{
public:
    static constexpr size_t LogsPerFrame = 100;
    static constexpr size_t ImGuiHistoryCount = 2000;

    static void Init();
    static void Flush();
    static void FlushAll();
    static void DrawImGui();
    static void Shutdown();

private:
    struct ImGuiLogEntry
    {
        LogEntry entry;
    };

    struct AggregationState;

    static size_t FlushBatch(size_t maxCount, bool forceFinalize);
    static void ProcessEntry(LogEntry entry);
    static void FinalizeCurrentLogs();

    static LogFileWorker s_fileWorker;
    static std::deque<ImGuiLogEntry> s_imguiEntries;
    static std::unique_ptr<AggregationState> s_aggregationState;
};
