#include "Script/EffectConfig.h"

#include "imgui.h"
#include "imgui_curve_custom.h"

void EffectConfig::OnStart()
{
}

void EffectConfig::Update()
{
    if( ImGui::BeginTabBar("EffectConfig"))
    {
        if (ImGui::BeginTabItem("BallConfig"))
        {
            BoundBallConfigImGui();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("PatternTrailConfig"))
        {
            TrailConfigImGui();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void EffectConfig::BoundBallConfigImGui()
{
    // ImGuiで設定を表示
    ImGui::PushItemWidth(100);
    ImGui::DragFloat("Bounciness", &m_boundBallConfig.bounciness, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Lifetime Min", &m_boundBallConfig.lifetimeMin, 0.1f, 0.1f, FLT_MAX);
    ImGui::DragFloat("Lifetime Max", &m_boundBallConfig.lifetimeMax, 0.1f, 0.1f, FLT_MAX);
    ImGui::DragFloat("Gravity", &m_boundBallConfig.gravity, 0.1f, FLT_MIN, FLT_MAX);
    ImGui::PopItemWidth();
    if (m_boundBallConfig.lifetimeMin > m_boundBallConfig.lifetimeMax)
    {
        std::swap(m_boundBallConfig.lifetimeMin, m_boundBallConfig.lifetimeMax);
    }
}

void EffectConfig::TrailConfigImGui()
{
    // ImGuiで設定を表示
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Position Curve"))
    {
        ImGui::EditCurveVector3("Position", m_patternTrailConfig.positionCurve);
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Color Curve"))
    {
        ImGui::EditCurveColor("Color", m_patternTrailConfig.colorCurve);
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Width Curve"))
    {
        ImGui::EditCurveFloat("Width", m_patternTrailConfig.widthCurve);
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::PushItemWidth(100);
    ImGui::DragInt("Amount", &m_patternTrailConfig.trailCount, 1, 1, 128);
    ImGui::DragFloat("Interval", &m_patternTrailConfig.spawnInterval, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat("Width", &m_patternTrailConfig.width, 0.01f, 0.01f, 1.0f);
    ImGui::DragFloat("Length", &m_patternTrailConfig.length, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat("Life", &m_patternTrailConfig.lifeTime, 0.01f, 0.1f, 10.0f);
    ImGui::PopItemWidth();
}
