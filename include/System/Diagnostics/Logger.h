#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

enum class LogLevel
{
    Log,
    Warning,
    Error,
    Fatal,
};

struct LogEntry
{
    LogLevel level;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::source_location source;
    std::thread::id threadId;
    size_t count = 1;
};

class LogSystem;

// 呼び出し側にはログの記録だけを公開し、出力タイミングはLogSystemへ委ねる。
class Logger final
{
public:
    static constexpr size_t MaxQueuedLogs = 4096;

    static void Log(
        std::string_view message,
        const std::source_location& source = std::source_location::current());

    static void Warning(
        std::string_view message,
        const std::source_location& source = std::source_location::current());

    static void Error(
        std::string_view message,
        const std::source_location& source = std::source_location::current());

    // 致命的な失敗を記録してから通知する。終了判断は呼び出し元が行う。
    static void Fatal(
        std::string_view message,
        const std::source_location& source = std::source_location::current());

private:
    friend class LogSystem;

    struct DrainResult
    {
        std::vector<LogEntry> entries;
        size_t droppedCount;
        bool hasMore;
    };

    static void Write(
        LogLevel level,
        std::string_view message,
        const std::source_location& source);

    static DrainResult Drain(size_t maxCount);
    static bool MakeRoom(LogLevel incomingLevel);

    static std::mutex s_mutex;
    static std::deque<LogEntry> s_entries;
    static size_t s_droppedCount;
};
