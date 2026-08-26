#include "pch.h"

#include "imgui.h"

#include "System/Editor/DebugWindow.h"
#include "System/Diagnostics/LogSystem.h"
#include "System/Diagnostics/PerformanceMonitor.h"


void DebugWindow::DrawImGui()
{
    if (BeginWindow("Debug", Vector2(700.0f, 350.0f)))
    {
        if (ImGui::BeginTabBar("DebugTabs"))
        {
            PerformanceMonitor::DrawImGuiTab();
            LogSystem::DrawImGuiTab();

            ImGui::EndTabBar();
        }
    }
    EndWindow();
}
