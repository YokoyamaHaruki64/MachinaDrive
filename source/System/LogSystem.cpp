#include "pch.h"

#include <iomanip>

#include "imgui.h"

#include "System/LogFileWorker.h"
#include "System/Logger.h"
#include "System/LogSystem.h"

LogFileWorker LogSystem::s_fileWorker;
std::deque<LogSystem::ImGuiLogEntry> LogSystem::s_imguiEntries;


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


bool IsVisibleLevel(LogLevel level, bool showLog, bool showWarning, bool showError)
{
    switch (level)
    {
    case LogLevel::Log:
        return showLog;
    case LogLevel::Warning:
        return showWarning;
    case LogLevel::Error:
        return showError;
    default:
        return false;
    }
}


ImVec4 GetLevelColor(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Warning:
        return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
    case LogLevel::Error:
        return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
    default:
        return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    }
}
}


void LogSystem::Init()
{
    if (!s_fileWorker.Start())
    {
        OutputDebugStringA("[LogSystem][Error] Logファイルを作成できませんでした。\n");
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


void LogSystem::DrawImGui()
{
    static bool showLog = true;
    static bool showWarning = true;
    static bool showError = true;
    static bool autoScroll = true;
    static ImGuiTextFilter textFilter;

    ImGui::SetNextWindowSize(ImVec2(700.0f, 350.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug"))
    {
        if (ImGui::BeginTabBar("DebugTabs"))
        {
            if (ImGui::BeginTabItem("Log"))
            {
                ImGui::Checkbox("Log", &showLog);
                ImGui::SameLine();
                ImGui::Checkbox("Warning", &showWarning);
                ImGui::SameLine();
                ImGui::Checkbox("Error", &showError);
                ImGui::SameLine();
                ImGui::Checkbox("Auto Scroll", &autoScroll);
                ImGui::SameLine();

                if (ImGui::Button("Clear"))
                {
                    s_imguiEntries.clear();
                }

                textFilter.Draw("Filter", 250.0f);

                if (ImGui::BeginChild("LogEntries", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar))
                {
                    const bool scrollToBottom = autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

                    for (const ImGuiLogEntry& entry : s_imguiEntries)
                    {
                        if (!IsVisibleLevel(entry.level, showLog, showWarning, showError)
                            || !textFilter.PassFilter(entry.text.c_str()))
                        {
                            continue;
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, GetLevelColor(entry.level));
                        ImGui::TextUnformatted(entry.text.c_str());
                        ImGui::PopStyleColor();
                    }

                    if (scrollToBottom)
                    {
                        ImGui::SetScrollHereY(1.0f);
                    }
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}


void LogSystem::Shutdown()
{
    FlushAll();
    s_fileWorker.Shutdown();
    s_imguiEntries.clear();
}


size_t LogSystem::FlushBatch(size_t maxCount)
{
    std::vector<LogEntry> entries = Logger::Drain(maxCount);
    std::vector<std::string> fileEntries;
    fileEntries.reserve(entries.size());

    for (const LogEntry& entry : entries)
    {
        std::string output = FormatEntry(entry);
        OutputDebugStringA(output.c_str());

        s_imguiEntries.emplace_back(ImGuiLogEntry{
            .level = entry.level,
            .text = output,
        });
        fileEntries.emplace_back(std::move(output));
    }

    while (s_imguiEntries.size() > ImGuiHistoryCount)
    {
        s_imguiEntries.pop_front();
    }

    s_fileWorker.Enqueue(std::move(fileEntries));

    return entries.size();
}
