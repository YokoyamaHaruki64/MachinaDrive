#include "pch.h"

#include "imgui.h"

#include "System/DebugWindow.h"
#include "System/EditorSystem.h"
#include "System/LogSystem.h"
#include "System/PerformanceMonitor.h"


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
