#include "pch.h"

#include <iomanip>

#include "System/LogFileWorker.h"

namespace
{
std::filesystem::path CreateLogFilePath()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
    localtime_s(&localTime, &time);

    std::ostringstream fileName;
    fileName
        << "Log_"
        << std::put_time(&localTime, "%Y%m%d_%H%M%S")
        << '_' << GetCurrentProcessId()
        << ".txt";

    return std::filesystem::path("Logs") / fileName.str();
}


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
    case LogLevel::Fatal:
        return "Fatal";
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
        << "][" << entry.count
        << "][" << GetFileName(entry.source.file_name())
        << ':' << entry.source.line()
        << "][Thread " << entry.threadId
        << "]\n    ";

    for (size_t i = 0; i < entry.message.size(); ++i)
    {
        const char character = entry.message[i];
        stream << character;
        if (character == '\n' && i + 1 < entry.message.size())
        {
            stream << "    ";
        }
    }
    stream << '\n';

    return stream.str();
}


std::deque<LogEntry>::iterator FindDiscardCandidate(
    std::deque<LogEntry>& entries,
    LogLevel incomingLevel)
{
    // ファイル待ちキューもLoggerと同じ優先順位で、古い低重要度ログから置き換える。
    auto candidate = std::find_if(entries.begin(), entries.end(), [](const LogEntry& entry)
    {
        return entry.level == LogLevel::Log;
    });

    if (candidate == entries.end() && incomingLevel != LogLevel::Log)
    {
        candidate = std::find_if(entries.begin(), entries.end(), [](const LogEntry& entry)
        {
            return entry.level == LogLevel::Warning;
        });
    }

    if (candidate == entries.end()
        && incomingLevel != LogLevel::Log
        && incomingLevel != LogLevel::Warning)
    {
        candidate = std::find_if(entries.begin(), entries.end(), [](const LogEntry& entry)
        {
            return entry.level == LogLevel::Error;
        });
    }

    if (candidate == entries.end() && incomingLevel == LogLevel::Fatal)
    {
        candidate = std::find_if(entries.begin(), entries.end(), [](const LogEntry& entry)
        {
            return entry.level == LogLevel::Fatal;
        });
    }

    return candidate;
}
}


LogFileWorker::~LogFileWorker()
{
    Shutdown();
}


bool LogFileWorker::Start()
{
    if (m_isRunning)
    {
        return true;
    }

    std::error_code error;
    std::filesystem::create_directories("Logs", error);
    if (error)
    {
        return false;
    }

    m_stream.open(CreateLogFilePath(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!m_stream.is_open())
    {
        return false;
    }

    m_stopRequested = false;
    m_isRunning = true;
    m_thread = std::thread(&LogFileWorker::Run, this);

    return true;
}


size_t LogFileWorker::Enqueue(std::vector<LogEntry> entries)
{
    size_t droppedCount = 0;

    if (entries.empty())
    {
        return droppedCount;
    }

    {
        std::lock_guard lock(m_mutex);
        if (!m_isRunning || m_stopRequested)
        {
            return entries.size();
        }

        for (LogEntry& entry : entries)
        {
            if (m_entries.size() >= MaxQueuedLogs)
            {
                auto candidate = FindDiscardCandidate(m_entries, entry.level);
                if (candidate == m_entries.end())
                {
                    ++droppedCount;
                    continue;
                }

                m_entries.erase(candidate);
                ++droppedCount;
            }

            m_entries.emplace_back(std::move(entry));
        }
    }

    m_condition.notify_one();

    return droppedCount;
}


void LogFileWorker::Shutdown()
{
    {
        std::lock_guard lock(m_mutex);
        if (!m_isRunning)
        {
            return;
        }

        m_stopRequested = true;
    }

    m_condition.notify_one();

    if (m_thread.joinable())
    {
        m_thread.join();
    }

    m_stream.flush();
    m_stream.close();
    m_isRunning = false;
}


void LogFileWorker::Run()
{
    auto lastFlushTime = std::chrono::steady_clock::now();

    while (true)
    {
        std::deque<LogEntry> entries;
        bool shouldStop = false;

        {
            std::unique_lock lock(m_mutex);
            m_condition.wait_for(lock, std::chrono::seconds(1), [this]
            {
                return m_stopRequested || !m_entries.empty();
            });

            shouldStop = m_stopRequested && m_entries.empty();

            // ファイルI/O中にメインスレッドを待たせないよう、書き込み対象をローカルへ移す。
            entries.swap(m_entries);
        }

        for (const LogEntry& entry : entries)
        {
            const std::string output = FormatEntry(entry);
            m_stream.write(output.data(), static_cast<std::streamsize>(output.size()));
        }

        const auto now = std::chrono::steady_clock::now();
        // 毎フレームのディスク同期を避けつつ、長時間バッファへ残さない間隔にする。
        if (now - lastFlushTime >= std::chrono::seconds(1))
        {
            m_stream.flush();
            lastFlushTime = now;
        }

        if (shouldStop)
        {
            break;
        }
    }
}
