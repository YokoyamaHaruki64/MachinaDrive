#include "pch.h"

#include "System/Editor/EditorWindow.h"

#include "imgui.h"

bool EditorWindow::BeginWindow(const char* title, const Vector2& initialSize)
{
    ImGui::SetNextWindowSize(ImVec2(initialSize.x, initialSize.y), ImGuiCond_FirstUseEver);
    const bool isVisible = ImGui::Begin(title);
    const ImVec2 windowSize = ImGui::GetWindowSize();
    const ImVec2 contentSize = ImGui::GetContentRegionAvail();
    m_windowSize = Vector2(windowSize.x, windowSize.y);
    m_contentSize = Vector2(contentSize.x, contentSize.y);
    return isVisible;
}

void EditorWindow::UpdateContentSize()
{
    const ImVec2 contentSize = ImGui::GetContentRegionAvail();
    m_contentSize = Vector2(contentSize.x, contentSize.y);

    if (m_layoutFrameCount < 2)
        ++m_layoutFrameCount;
}

void EditorWindow::EndWindow()
{
    ImGui::End();
}
