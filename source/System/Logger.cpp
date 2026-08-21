#include "pch.h"

#include "System/Logger.h"

std::mutex Logger::s_mutex;
std::deque<LogEntry> Logger::s_entries;


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
    };

    std::lock_guard lock(s_mutex);
    s_entries.emplace_back(std::move(entry));
}


std::vector<LogEntry> Logger::Drain(size_t maxCount)
{
    std::vector<LogEntry> entries;

    std::lock_guard lock(s_mutex);
    const size_t outputCount = std::min(maxCount, s_entries.size());
    entries.reserve(outputCount);

    for (size_t i = 0; i < outputCount; ++i)
    {
        entries.emplace_back(std::move(s_entries.front()));
        s_entries.pop_front();
    }

    return entries;
}
