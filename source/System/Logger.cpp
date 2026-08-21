#include "pch.h"

#include <utility>

#include "System/Logger.h"

std::mutex Logger::s_mutex;
std::deque<LogEntry> Logger::s_entries;
size_t Logger::s_droppedCount = 0;


void Logger::Log(std::string_view message, const std::source_location& source)
{
    Write(LogLevel::Log, message, source);
}


void Logger::Warning(std::string_view message, const std::source_location& source)
{
    Write(LogLevel::Warning, message, source);
}


void Logger::Error(std::string_view message, const std::source_location& source)
{
    Write(LogLevel::Error, message, source);
}


void Logger::Write(
    LogLevel level,
    std::string_view message,
    const std::source_location& source)
{
    LogEntry entry{
        .level = level,
        .message = std::string(message),
        .timestamp = std::chrono::system_clock::now(),
        .source = source,
        .threadId = std::this_thread::get_id(),
        .count = 1,
    };

    std::lock_guard lock(s_mutex);
    if (s_entries.size() >= MaxQueuedLogs && !MakeRoom(level))
    {
        ++s_droppedCount;
        return;
    }

    s_entries.emplace_back(std::move(entry));
}


Logger::DrainResult Logger::Drain(size_t maxCount)
{
    DrainResult result{};

    std::lock_guard lock(s_mutex);
    const size_t outputCount = std::min(maxCount, s_entries.size());
    result.entries.reserve(outputCount);

    for (size_t i = 0; i < outputCount; ++i)
    {
        result.entries.emplace_back(std::move(s_entries.front()));
        s_entries.pop_front();
    }

    result.droppedCount = std::exchange(s_droppedCount, 0);
    result.hasMore = !s_entries.empty();

    return result;
}


bool Logger::MakeRoom(LogLevel incomingLevel)
{
    // 新しい重要ログを残すため、上限到達時は古い低重要度ログから置き換える。
    auto candidate = std::find_if(s_entries.begin(), s_entries.end(), [](const LogEntry& entry)
    {
        return entry.level == LogLevel::Log;
    });

    // WarningでErrorを、LogでWarning以上を追い出さない。
    if (candidate == s_entries.end() && incomingLevel != LogLevel::Log)
    {
        candidate = std::find_if(s_entries.begin(), s_entries.end(), [](const LogEntry& entry)
        {
            return entry.level == LogLevel::Warning;
        });
    }

    if (candidate == s_entries.end() && incomingLevel == LogLevel::Error)
    {
        candidate = std::find_if(s_entries.begin(), s_entries.end(), [](const LogEntry& entry)
        {
            return entry.level == LogLevel::Error;
        });
    }

    if (candidate == s_entries.end())
    {
        // 既存ログの方が重要な場合は新しいログを破棄する。
        return false;
    }

    s_entries.erase(candidate);
    ++s_droppedCount;

    return true;
}
