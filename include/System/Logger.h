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
};

struct LogEntry
{
    LogLevel level;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    std::source_location source;
    std::thread::id threadId;
};

class LogSystem;

// 呼び出し側にはログの記録だけを公開し、出力タイミングはLogSystemへ委ねる。
class Logger final
{
public:
    static void Log(
        std::string_view message,
        const std::source_location& source = std::source_location::current());

    static void Warning(
        std::string_view message,
        const std::source_location& source = std::source_location::current());

    static void Error(
        std::string_view message,
        const std::source_location& source = std::source_location::current());

private:
    friend class LogSystem;

    static void Write(
        LogLevel level,
        std::string_view message,
        const std::source_location& source);

    static std::vector<LogEntry> Drain(size_t maxCount);

    static std::mutex s_mutex;
    static std::deque<LogEntry> s_entries;
};
