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


void LogFileWorker::Enqueue(std::vector<std::string> entries)
{
    if (entries.empty())
    {
        return;
    }

    {
        std::lock_guard lock(m_mutex);
        if (!m_isRunning || m_stopRequested)
        {
            return;
        }

        for (std::string& entry : entries)
        {
            m_entries.emplace_back(std::move(entry));
        }
    }

    m_condition.notify_one();
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
    while (true)
    {
        std::deque<std::string> entries;

        {
            std::unique_lock lock(m_mutex);
            m_condition.wait(lock, [this]
            {
                return m_stopRequested || !m_entries.empty();
            });

            if (m_stopRequested && m_entries.empty())
            {
                break;
            }

            entries.swap(m_entries);
        }

        for (const std::string& entry : entries)
        {
            m_stream.write(entry.data(), static_cast<std::streamsize>(entry.size()));
        }
        m_stream.flush();
    }
}
