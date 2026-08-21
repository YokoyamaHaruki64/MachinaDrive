#pragma once

#include <cstddef>
#include <deque>
#include <string>

enum class LogLevel;
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
        LogLevel level;
        std::string text;
    };

    static size_t FlushBatch(size_t maxCount);

    static LogFileWorker s_fileWorker;
    static std::deque<ImGuiLogEntry> s_imguiEntries;
};
