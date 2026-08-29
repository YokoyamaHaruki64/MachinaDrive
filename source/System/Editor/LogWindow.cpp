#include "pch.h"

#include "System/Editor/LogWindow.h"
#include "System/Diagnostics/LogSystem.h"

void LogWindow::DrawImGui()
{
    if (BeginWindow("Log", Vector2(720.0f, 320.0f)))
    {
        LogSystem::DrawImGui();
    }
    EndWindow();
}
