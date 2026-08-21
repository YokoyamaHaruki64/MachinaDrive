#pragma once

#include <condition_variable>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ファイルI/Oをメインスレッドから分離し、受け取ったログを順番に書き出す。
class LogFileWorker final
{
public:
    LogFileWorker() = default;
    ~LogFileWorker();

    LogFileWorker(const LogFileWorker&) = delete;
    LogFileWorker& operator=(const LogFileWorker&) = delete;

    bool Start();
    void Enqueue(std::vector<std::string> entries);
    void Shutdown();

private:
    void Run();

    std::ofstream m_stream;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::deque<std::string> m_entries;
    bool m_isRunning = false;
    bool m_stopRequested = false;
};
