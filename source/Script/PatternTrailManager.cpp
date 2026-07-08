#include "pch.h"

#include "Script/PatternTrailManager.h"

#include "System/Manager.h"
#include "System/GM_Time.h"
#include "Component/Transform.h"
#include "Common/Random.h"

#include "Script/Wall.h"
#include "Script/PatternTrail.h"

#include "imgui.h"
#include "imgui_curve_custom.h"

void PatternTrailManager::OnStart()
{
    m_walls = Manager::GetObjectsByComponent<Wall>();
}

void PatternTrailManager::Update()
{
    m_elapsedTime += Time::GetDeltaTime();
    if (m_elapsedTime >= m_spawnInterval)
    {
        m_elapsedTime -= m_spawnInterval;
        CreatePatternTrail_Random();
    }

    // ImGuiで設定を表示
    ImGui::Begin("PatternTrail Settings");

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if( ImGui::TreeNode("Position Curve"))
    {
        ImGui::EditCurveVector3("Position", m_settings.positionCurve);
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if( ImGui::TreeNode("Color Curve"))
    {
        ImGui::EditCurveColor("Color", m_settings.colorCurve);
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if( ImGui::TreeNode("Width Curve"))
    {
        ImGui::EditCurveFloat("Width", m_settings.widthCurve);
        ImGui::TreePop();
    }
    ImGui::Separator();
    ImGui::PushItemWidth(100);
    ImGui::DragInt("Amount", &m_trailCount, 1, 1, 128);
    ImGui::SameLine();
    ImGui::DragFloat("Interval", &m_spawnInterval, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat("Width", &m_settings.width, 0.01f, 0.01f, 1.0f);
    ImGui::SameLine();
    ImGui::DragFloat("Length", &m_settings.length, 0.01f, 0.1f, 5.0f);
    ImGui::DragFloat("Life", &m_settings.lifeTime, 0.01f, 0.1f, 10.0f);
    ImGui::PopItemWidth();

    ImGui::End();
}

void PatternTrailManager::CreatePatternTrail_Random()
{
    if (m_walls.empty()) return;

    // 壁からランダムな位置を生成
    // intのRangeはmaxが含まれないためsize()をそのまま渡す
    int wallIndex = Random::Range(0, static_cast<int>(m_walls.size() - 1));
    auto wall = m_walls[wallIndex];
    Vector3 wallPos = wall->GetTransform()->GetPosition();
    Vector3 wallScale = wall->GetTransform()->GetScale();
    wallScale *= 0.5f; // 少し小さくして壁の端に出ないようにする
    Vector3 normal = wall->GetComponent<Wall>()->GetNormal();
    Vector3 randomPos = wallPos;

    // normal が向いている軸を「厚み軸」とする
    int thicknessAxis = 0;

    Vector3 absNormal(
        std::abs(normal.x),
        std::abs(normal.y),
        std::abs(normal.z)
    );

    // 厚み軸を決定
    if (absNormal.y > absNormal.x && absNormal.y > absNormal.z)
    {
        thicknessAxis = 1;
    }
    else if (absNormal.z > absNormal.x && absNormal.z > absNormal.y)
    {
        thicknessAxis = 2;
    }

    // 各軸の単位ベクトル
    Vector3 axes[3] =
    {
        Vector3::Right(),
        Vector3::Up(),
        Vector3::Forward()
    };

    // 各軸のスケール
    float scales[3] =
    {
        wallScale.x,
        wallScale.y,
        wallScale.z
    };

    // 厚み軸以外の2軸にランダムオフセット
    for (int i = 0; i < 3; ++i)
    {
        if (i == thicknessAxis)
            continue;

        float offset = Random::Range(-scales[i] * 0.5f, scales[i] * 0.5f);
        randomPos += axes[i] * offset;
    }

    // normal方向に少し押し出す
    randomPos += normal * scales[thicknessAxis] * 0.75f;

    // PatternTrailを生成
    GameObject* patternTrailObj = Manager::CreateEmpty(randomPos);
    auto patternTrailComp = patternTrailObj->AddComponent<PatternTrail>();
    patternTrailComp->CreateTrailPattern(randomPos, normal, m_trailCount);
}
