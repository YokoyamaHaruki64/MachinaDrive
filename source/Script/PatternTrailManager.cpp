#include "pch.h"

#include "Component/ComponentInc.h"

#include "Script/PatternTrailManager.h"

#include "Common/Random.h"

#include "Script/Wall.h"
#include "Script/PatternTrail.h"
#include "Script/EffectConfig.h"

void PatternTrailManager::OnStart()
{
    for (GameObject* wall : Manager::GetObjectsByComponent<Wall>())
    {
        m_walls.emplace_back(wall);
    }
    m_config = Manager::GetFirstObjectByComponent<EffectConfig>()->GetComponent<EffectConfig>();
}

void PatternTrailManager::Update()
{
    m_elapsedTime += Time::GetDeltaTime();
    if (m_elapsedTime >= m_config->GetPatternTrailConfig().spawnInterval)
    {
        m_elapsedTime -= m_config->GetPatternTrailConfig().spawnInterval;
        CreatePatternTrail_Random();
    }

}

void PatternTrailManager::CreatePatternTrail_Random()
{
    if (m_walls.empty()) return;

    // 壁からランダムな位置を生成
    // intのRangeはmaxが含まれないためsize()をそのまま渡す
    int wallIndex = Random::Range(0, static_cast<int>(m_walls.size() - 1));
    const GameObjectReference& wall = m_walls[wallIndex];
    if (!wall)
    {
        return;
    }
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
    patternTrailComp->CreateTrailPattern(randomPos, normal, m_config->GetPatternTrailConfig().trailCount);
}
