#include "pch.h"

#include "System/Editor/PerformanceWindow.h"
#include "System/Diagnostics/PerformanceMonitor.h"

void PerformanceWindow::DrawImGui()
{
    if (BeginWindow("Performance", Vector2(440.0f, 260.0f)))
    {
        PerformanceMonitor::DrawImGui();
    }
    EndWindow();
}
