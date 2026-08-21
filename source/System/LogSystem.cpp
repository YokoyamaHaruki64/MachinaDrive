#include "pch.h"

#include <iomanip>

#include "System/Logger.h"
#include "System/LogSystem.h"

namespace
{
const char* GetLevelName(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Log:
        return "Log";
    case LogLevel::Warning:
        return "Warning";
    case LogLevel::Error:
        return "Error";
    default:
        return "Unknown";
    }
}


std::string_view GetFileName(std::string_view filePath)
{
    const size_t separatorIndex = filePath.find_last_of("/\\");
    if (separatorIndex == std::string_view::npos)
    {
        return filePath;
    }

    return filePath.substr(separatorIndex + 1);
}


std::string FormatEntry(const LogEntry& entry)
{
    const std::time_t time = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()) % 1000;

    std::ostringstream stream;
    stream
        << '[' << std::put_time(&localTime, "%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << milliseconds.count()
        << "][" << GetLevelName(entry.level)
        << "][" << GetFileName(entry.source.file_name())
        << ':' << entry.source.line()
        << "][Thread " << entry.threadId
        << "] " << entry.message
        << '\n';

    return stream.str();
}
}


void LogSystem::Flush()
{
    FlushBatch(LogsPerFrame);
}


void LogSystem::FlushAll()
{
    while (FlushBatch(LogsPerFrame) > 0)
    {
    }
}


size_t LogSystem::FlushBatch(size_t maxCount)
{
    std::vector<LogEntry> entries = Logger::Drain(maxCount);

    for (const LogEntry& entry : entries)
    {
        const std::string output = FormatEntry(entry);
        OutputDebugStringA(output.c_str());
    }

    return entries.size();
}
