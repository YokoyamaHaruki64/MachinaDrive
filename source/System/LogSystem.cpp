#include "pch.h"

#include <iomanip>
#include <optional>

#include "imgui.h"

#include "System/LogFileWorker.h"
#include "System/LogSystem.h"

namespace
{
struct LogSourceKey
{
    LogLevel level;
    std::string_view file;
    uint_least32_t line;
    std::thread::id threadId;

    bool operator==(const LogSourceKey&) const = default;
};


struct LogSourceKeyHasher
{
    size_t operator()(const LogSourceKey& key) const
    {
        size_t hash = std::hash<std::string_view>{}(key.file);
        hash ^= std::hash<uint_least32_t>{}(key.line) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<std::thread::id>{}(key.threadId) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int>{}(static_cast<int>(key.level)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

        return hash;
    }
};


struct MessageLookup
{
    std::unordered_map<size_t, std::vector<size_t>> entryIndicesByHash;
};


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


bool IsVisibleLevel(
    LogLevel level,
    bool showLog,
    bool showWarning,
    bool showError,
    bool showFatal)
{
    switch (level)
    {
    case LogLevel::Log:
        return showLog;
    case LogLevel::Warning:
        return showWarning;
    case LogLevel::Error:
        return showError;
    case LogLevel::Fatal:
        return showFatal;
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
    case LogLevel::Fatal:
        return ImVec4(1.0f, 0.2f, 1.0f, 1.0f);
    default:
        return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    }
}


int64_t GetLogSecond(const std::chrono::system_clock::time_point& timestamp)
{
    return std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count();
}
}


struct LogSystem::AggregationState
{
    std::optional<int64_t> currentSecond;
    std::vector<LogEntry> currentEntries;
    std::unordered_map<LogSourceKey, MessageLookup, LogSourceKeyHasher> sourceLookup;
    size_t loggerDroppedCount = 0;
    size_t fileDroppedCount = 0;
};

LogFileWorker LogSystem::s_fileWorker;
std::deque<LogSystem::ImGuiLogEntry> LogSystem::s_imguiEntries;
std::unique_ptr<LogSystem::AggregationState> LogSystem::s_aggregationState;


void LogSystem::Init()
{
    s_aggregationState = std::make_unique<AggregationState>();

    if (!s_fileWorker.Start())
    {
        Logger::Error("Logファイルを作成できませんでした。");
    }
}


void LogSystem::Flush()
{
    FlushBatch(LogsPerFrame, false);
}


void LogSystem::FlushAll()
{
    while (FlushBatch(LogsPerFrame, false) > 0)
    {
    }

    FinalizeCurrentLogs();
}


void LogSystem::DrawImGuiTab()
{
    if (!s_aggregationState)
    {
        return;
    }

    static bool showLog = true;
    static bool showWarning = true;
    static bool showError = true;
    static bool showFatal = true;
    static bool autoScroll = true;
    static ImGuiTextFilter textFilter;

    if (ImGui::BeginTabItem("Log"))
    {
        ImGui::Checkbox("Log", &showLog);
        ImGui::SameLine();
        ImGui::Checkbox("Warning", &showWarning);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &showError);
        ImGui::SameLine();
        ImGui::Checkbox("Fatal", &showFatal);
        ImGui::SameLine();
        ImGui::Checkbox("Auto Scroll", &autoScroll);
        ImGui::SameLine();

        if (ImGui::Button("Clear"))
        {
            s_imguiEntries.clear();
        }

        textFilter.Draw("Filter", 250.0f);
        ImGui::SameLine();
        ImGui::Text(
            "Dropped: Logger %zu / File %zu",
            s_aggregationState->loggerDroppedCount,
            s_aggregationState->fileDroppedCount);

        if (ImGui::BeginChild("LogEntries", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            const bool scrollToBottom = autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY();
            const size_t finalizedCount = s_imguiEntries.size();
            const size_t totalCount = finalizedCount + s_aggregationState->currentEntries.size();

            const auto getEntry = [finalizedCount](size_t index) -> const LogEntry&
            {
                if (index < finalizedCount)
                {
                    return s_imguiEntries[index].entry;
                }

                return s_aggregationState->currentEntries[index - finalizedCount];
            };

            const auto drawEntry = [&getEntry](size_t index)
            {
                const LogEntry& entry = getEntry(index);
                const std::string text = FormatEntry(entry);

                ImGui::PushStyleColor(ImGuiCol_Text, GetLevelColor(entry.level));
                ImGui::TextUnformatted(text.c_str());
                ImGui::PopStyleColor();
            };

            const bool requiresFiltering = textFilter.IsActive()
                || !showLog
                || !showWarning
                || !showError
                || !showFatal;

            // 絞り込み中は表示対象が不連続になるため、全件から対象を選ぶ。
            if (requiresFiltering)
            {
                for (size_t i = 0; i < totalCount; ++i)
                {
                    const LogEntry& entry = getEntry(i);
                    if (!IsVisibleLevel(entry.level, showLog, showWarning, showError, showFatal))
                    {
                        continue;
                    }

                    const std::string text = FormatEntry(entry);
                    if (!textFilter.PassFilter(text.c_str()))
                    {
                        continue;
                    }

                    ImGui::PushStyleColor(ImGuiCol_Text, GetLevelColor(entry.level));
                    ImGui::TextUnformatted(text.c_str());
                    ImGui::PopStyleColor();
                }
            }
            else
            {
                // 全レベル表示時は画面内の項目だけを整形・描画する。
                ImGuiListClipper clipper;
                clipper.Begin(static_cast<int>(totalCount));
                while (clipper.Step())
                {
                    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                    {
                        const LogEntry& entry = getEntry(static_cast<size_t>(i));
                        if (IsVisibleLevel(entry.level, showLog, showWarning, showError, showFatal))
                        {
                            drawEntry(static_cast<size_t>(i));
                        }
                    }
                }
            }

            if (scrollToBottom)
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
        ImGui::EndTabItem();
    }
}


void LogSystem::Shutdown()
{
    FlushAll();
    s_fileWorker.Shutdown();
    s_imguiEntries.clear();
    s_aggregationState.reset();
}


size_t LogSystem::FlushBatch(size_t maxCount, bool forceFinalize)
{
    if (!s_aggregationState)
    {
        return 0;
    }

    Logger::DrainResult result = Logger::Drain(maxCount);
    s_aggregationState->loggerDroppedCount += result.droppedCount;

    for (LogEntry& entry : result.entries)
    {
        ProcessEntry(std::move(entry));
    }

    if (forceFinalize)
    {
        FinalizeCurrentLogs();
    }
    else if (!result.hasMore && s_aggregationState->currentSecond)
    {
        // キューに旧秒の残件がある間は、同じ秒のログが分割されるため確定しない。
        const int64_t nowSecond = GetLogSecond(std::chrono::system_clock::now());
        if (*s_aggregationState->currentSecond != nowSecond)
        {
            FinalizeCurrentLogs();
        }
    }

    return result.entries.size();
}


void LogSystem::ProcessEntry(LogEntry entry)
{
    const int64_t entrySecond = GetLogSecond(entry.timestamp);
    // 秒が変わった時点で前秒の件数を確定し、ファイル出力へ渡す。
    if (s_aggregationState->currentSecond
        && *s_aggregationState->currentSecond != entrySecond)
    {
        FinalizeCurrentLogs();
    }

    if (!s_aggregationState->currentSecond)
    {
        s_aggregationState->currentSecond = entrySecond;
    }

    const LogSourceKey sourceKey{
        .level = entry.level,
        .file = entry.source.file_name(),
        .line = entry.source.line(),
        .threadId = entry.threadId,
    };

    // 発生元を先に絞り、その中でメッセージをハッシュ検索する二段階構成とする。
    MessageLookup& messageLookup = s_aggregationState->sourceLookup[sourceKey];
    const size_t messageHash = std::hash<std::string_view>{}(entry.message);
    std::vector<size_t>& candidateIndices = messageLookup.entryIndicesByHash[messageHash];

    for (const size_t candidateIndex : candidateIndices)
    {
        LogEntry& candidate = s_aggregationState->currentEntries[candidateIndex];
        // ハッシュ衝突時に異なる本文をまとめないよう、最後に文字列を照合する。
        if (candidate.message == entry.message)
        {
            candidate.count += entry.count;
            return;
        }
    }

    candidateIndices.push_back(s_aggregationState->currentEntries.size());
    s_aggregationState->currentEntries.emplace_back(std::move(entry));
}


void LogSystem::FinalizeCurrentLogs()
{
    if (!s_aggregationState || s_aggregationState->currentEntries.empty())
    {
        return;
    }

    std::vector<LogEntry> finalizedEntries = std::move(s_aggregationState->currentEntries);
    s_aggregationState->currentEntries.clear();
    s_aggregationState->sourceLookup.clear();
    s_aggregationState->currentSecond.reset();

    // ImGuiは履歴を保持し、Workerには所有権を移して以降の整形とI/Oを任せる。
    for (const LogEntry& entry : finalizedEntries)
    {
        s_imguiEntries.emplace_back(ImGuiLogEntry{ .entry = entry });
    }

    while (s_imguiEntries.size() > ImGuiHistoryCount)
    {
        s_imguiEntries.pop_front();
    }

    s_aggregationState->fileDroppedCount += s_fileWorker.Enqueue(std::move(finalizedEntries));
}
