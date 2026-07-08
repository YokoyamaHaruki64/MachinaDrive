#include "Script/EffectConfig.h"

#include "imgui.h"
#include "imgui_curve_custom.h"
#include "System/GM_time.h"

void EffectConfig::OnStart()
{
}

void EffectConfig::Update()
{
    float fps = 1.0f / Time::GetDeltaTime();
    ImGui::Text("FPS: %.1f", fps);

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
        if( ImGui::BeginTabItem("FireWorkConfig"))
        {
            FireWorkConfigImGui();
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
    ImGui::SetNextItemWidth(200);
    ImGui::DragFloat2("Life Range", &m_boundBallConfig.lifetimeMin, 0.1f, 0.1f, FLT_MAX);
    float min = m_boundBallConfig.lifetimeMin;
    float max = m_boundBallConfig.lifetimeMax;
    if (min > max) m_boundBallConfig.lifetimeMin = max;
    if (max < min) m_boundBallConfig.lifetimeMax = min;
    ImGui::DragFloat("Gravity", &m_boundBallConfig.gravity, 0.1f, FLT_MIN, FLT_MAX);
    ImGui::PopItemWidth();
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

void EffectConfig::FireWorkConfigImGui()
{
    ImGui::PushItemWidth(100);

    ImGui::DragInt("Burst Count", &m_fireWorkConfig.burstCount, 1, 1, 10000);
    ImGui::DragFloat("Drag", &m_fireWorkConfig.drag, 0.01f, 0.0f, 100.0f);
    ImGui::DragFloat("Gravity", &m_fireWorkConfig.gravity, 0.1f, -FLT_MAX, FLT_MAX);
    ImGui::DragFloat("Size", &m_fireWorkConfig.size, 0.01f, 0.01f, 10.0f);
    ImGui::PushItemWidth(200);

    ImGui::DragFloat2("Life Range", &m_fireWorkConfig.lifetimeMin, 0.1f, 0.1f, FLT_MAX);
    float min = m_fireWorkConfig.lifetimeMin;
    float max = m_fireWorkConfig.lifetimeMax;
    if(min > max) m_fireWorkConfig.lifetimeMin = max;
    if(max < min) m_fireWorkConfig.lifetimeMax = min;

    ImGui::DragFloat2("Speed Range", &m_fireWorkConfig.speedMin, 0.1f, 0.1f, FLT_MAX);
    min = m_fireWorkConfig.speedMin;
    max = m_fireWorkConfig.speedMax;
    if (min > max) m_fireWorkConfig.speedMin = max;
    if (max < min) m_fireWorkConfig.speedMax = min;


    ImGui::PopItemWidth();
    ImGui::PopItemWidth();

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Color Curve"))
    {
        ImGui::EditCurveColor("Color", m_fireWorkConfig.colorCurve);
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Scale Curve"))
    {
        ImGui::EditCurveFloat("Scale", m_fireWorkConfig.scaleCurve);
        ImGui::TreePop();
    }
}
