#include "pch.h"

#include "imgui.h"

#include "System/Editor/DebugWindow.h"
#include "System/Editor/EditorSystem.h"
#include "System/Diagnostics/LogSystem.h"
#include "System/Diagnostics/PerformanceMonitor.h"


void DebugWindow::DrawImGui()
{
    EditorSystem::DrawImGui();
    ImGui::SetNextWindowSize(ImVec2(700.0f, 350.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Debug"))
    {
        if (ImGui::BeginTabBar("DebugTabs"))
        {
            PerformanceMonitor::DrawImGuiTab();
            LogSystem::DrawImGuiTab();

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}
