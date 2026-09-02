#include "pch.h"

#include "System/Editor/GameWindow.h"

#include "imgui.h"

#include "System/Core/manager.h"
#include "System/Rendering/renderer.h"

namespace
{
    constexpr uint32_t MIN_GAME_SIZE = 1;
    constexpr uint32_t MAX_GAME_SIZE = 8192;

    uint32_t ToRenderSize(float value)
    {
        const float clamped = std::clamp(value, static_cast<float>(MIN_GAME_SIZE), static_cast<float>(MAX_GAME_SIZE));
        return static_cast<uint32_t>(std::lround(clamped));
    }
}

bool GameWindow::ApplyRenderTargetSize()
{
    if (!IsLayoutReady() || m_displaySize.x <= 0.0f || m_displaySize.y <= 0.0f)
        return false;

    return Renderer::ResizeGameRenderTarget(
        ToRenderSize(m_displaySize.x),
        ToRenderSize(m_displaySize.y));
}

bool GameWindow::IsRenderTargetReady() const
{
    return IsLayoutReady() &&
        ToRenderSize(m_displaySize.x) == Renderer::GetGameWidth() &&
        ToRenderSize(m_displaySize.y) == Renderer::GetGameHeight();
}

void GameWindow::DrawImGui()
{
    m_viewportHovered = false;
    if (!BeginWindow("Game", Vector2(800.0f, 500.0f)))
    {
        EndWindow();
        return;
    }

    if (Manager::IsPlayMode())
    {
        if (ImGui::Button("Stop"))
        {
            Manager::StopPlayMode();
        }
    }
    else if (ImGui::Button("Play"))
    {
        Manager::EnterPlayMode();
    }

    ImGui::SameLine();
    if (!Manager::IsPlayMode() && ImGui::Button(m_displayUI ? "Hide UI" : "Display UI"))
    {
        m_displayUI = !m_displayUI;
    }

    ImGui::SameLine();
    m_aspectMode = (Manager::IsPlayMode() || m_displayUI) ? AspectMode::Ratio16x9 : AspectMode::Free;
    ImGui::TextUnformatted(m_aspectMode == AspectMode::Ratio16x9 ? "16:9" : "Free");
    ImGui::SameLine();
    ImGui::Text("Render: %u x %u", Renderer::GetGameWidth(), Renderer::GetGameHeight());

    UpdateContentSize();
    const Vector2& contentSize = GetContentSize();
    float displayWidth = std::max(contentSize.x, 1.0f);
    float displayHeight = std::max(contentSize.y, 1.0f);
    if (m_aspectMode == AspectMode::Ratio16x9)
    {
        constexpr float aspectRatio = 16.0f / 9.0f;
        if (displayWidth / displayHeight > aspectRatio)
            displayWidth = displayHeight * aspectRatio;
        else
            displayHeight = displayWidth / aspectRatio;
    }

    const ImVec2 cursorPosition = ImGui::GetCursorScreenPos();
    const ImVec2 imagePosition(
        cursorPosition.x + (contentSize.x - displayWidth) * 0.5f,
        cursorPosition.y + (contentSize.y - displayHeight) * 0.5f);
    m_displayPosition = Vector2(imagePosition.x, imagePosition.y);
    m_displaySize = Vector2(displayWidth, displayHeight);

    ImGui::SetCursorScreenPos(imagePosition);
    ImGui::Image(
        static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(Renderer::GetSrcSRV())),
        ImVec2(displayWidth, displayHeight),
        ImVec2(0.0f, 0.0f),
        ImVec2(1.0f, 1.0f));
    m_viewportHovered = ImGui::IsItemHovered();

    EndWindow();
}
