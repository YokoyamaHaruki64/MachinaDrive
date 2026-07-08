
#include "pch.h"

#include "System/Manager.h" 
#include "System/GM_Time.h"
#include "Component/Transform.h"
#include "Common/Random.h"
#include "Script/BallManager.h"
#include "Script/Wall.h"
#include "Script/BoundBall.h"

#include "Script/PatternTrail.h"

#include "imgui.h"

void BallManager::OnStart()
{
    m_walls = Manager::GetObjectsByComponent<Wall>();
}

void BallManager::Update()
{
    ImGui::Begin("BallConfig");
    ImGui::DragFloat("Bounciness", &m_boundBallConfig.bounciness, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Lifetime Min", &m_boundBallConfig.lifetimeMin, 0.1f, 0.1f, FLT_MAX);
    ImGui::DragFloat("Lifetime Max", &m_boundBallConfig.lifetimeMax, 0.1f, 0.1f, FLT_MAX);
    ImGui::DragFloat("Gravity", &m_boundBallConfig.gravity, 0.1f, FLT_MIN, FLT_MAX);

    if( ImGui::Button("Random"))
    {
        CreateBoll_Random();
    }
    ImGui::SameLine();
    if( ImGui::Button("Center"))
    {
        CreateBoll(Vector3::Zero(), Vector3::Up());
    }

    if( m_boundBallConfig.lifetimeMin > m_boundBallConfig.lifetimeMax)
    {
        std::swap(m_boundBallConfig.lifetimeMin, m_boundBallConfig.lifetimeMax);
    }

    ImGui::End();
}

void BallManager::CreateBoll(Vector3 position,Vector3 normal)
{
    if (m_ballCount >= MAX_BALL_COUNT) return;

    // ボールを生成
    Vector3 randomVelocity = Random::RandomCone(normal, 30.0f) * 10.0f;
    Manager::Instantiate(
        "Ball",
        position,
        Vector3::Zero(),
        Vector3(1.0f, 1.0f, 1.0f)
    )->GetComponent<BoundBall>()->SetVelocity(randomVelocity);
    m_ballCount++;
}

void BallManager::CreateBoll_Random()
{
    if (m_ballCount >= MAX_BALL_COUNT) return;

    // 壁からランダムな位置を生成
    // intのRangeはmaxが含まれないためsize()をそのまま渡す
    int wallIndex = Random::Range(0, static_cast<int>(m_walls.size() - 1));
    auto wall = m_walls[wallIndex];

    Vector3 wallPos = wall->GetTransform()->GetPosition();
    Vector3 wallScale = wall->GetTransform()->GetScale();
    wallScale *= 0.8f; // 少し小さくして壁の端に出ないようにする
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

    // ボールを生成
    Vector3 randomVelocity = Random::RandomCone(normal, 30.0f) * 10.0f;
    Manager::Instantiate(
        "Ball",
        randomPos,
        Vector3::Zero(),
        Vector3(1.0f, 1.0f, 1.0f)
    )->GetComponent<BoundBall>()->SetVelocity(randomVelocity);

    m_ballCount++;
}
