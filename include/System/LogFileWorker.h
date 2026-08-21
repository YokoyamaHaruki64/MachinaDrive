#pragma once

#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <thread>
#include <vector>

#include "System/Logger.h"

// ファイルI/Oをメインスレッドから分離し、受け取ったログを順番に書き出す。
class LogFileWorker final
{
public:
    static constexpr size_t MaxQueuedLogs = 4096;

    LogFileWorker() = default;
    ~LogFileWorker();

    LogFileWorker(const LogFileWorker&) = delete;
    LogFileWorker& operator=(const LogFileWorker&) = delete;

    bool Start();
    size_t Enqueue(std::vector<LogEntry> entries);
    void Shutdown();

private:
    void Run();

    std::ofstream m_stream;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<LogEntry> m_entries;
    bool m_isRunning = false;
    bool m_stopRequested = false;
};
