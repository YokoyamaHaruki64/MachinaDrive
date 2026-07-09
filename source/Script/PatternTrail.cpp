#include "pch.h"

#include "System/Manager.h"

#include "Script/PatternTrail.h"
#include "Script/PatternTrailHeader.h"
#include "Script/BallManager.h"
#include "Script/EffectConfig.h"
#include "Component/TrailRenderer.h"
#include "Component/Transform.h"
#include "Component/Primitive.h"

void PatternTrail::OnStart()
{
    m_config = Manager::GetFirstObjectByComponent<EffectConfig>()->GetComponent<EffectConfig>();
}

void PatternTrail::Update()
{

    // 生成後、TrailHeaderが全て破棄されたら、自身も破棄
    if (m_trailHeaders.empty() && m_isCreated)
    {
        GameObject* ballManager = Manager::GetFirstObjectByComponent<BallManager>();
        if (ballManager)
        {
            ballManager->GetComponent<BallManager>()->
                CreateBoll(transform()->GetPosition(),m_normal);
        }

        m_owner->Destroy();
    }
}

void PatternTrail::LateUpdate()
{
    // 破棄されたトレイルヘッダーを削除
    for (int i = 0; i < m_trailHeaders.size(); ++i)
    {
        GameObject* trail = m_trailHeaders[i];
        if (!trail || trail->IsDestroyed())
        {
            // スワップポップで削除
            std::swap(m_trailHeaders[i], m_trailHeaders.back());
            m_trailHeaders.pop_back();
            --i;
        }
    }

}

void PatternTrail::CreateTrailPattern(
    const Vector3& position,
    const Vector3& normal,
    int trailCount)
{
    auto& settings = m_config->GetPatternTrailConfig();

    float angleStep = 360.0f / static_cast<float>(trailCount);
    Vector3 right;
    Vector3 up;
    Vector3::CreateRightUp(normal, right, up);

    for ( int i = 0; i < trailCount; ++i)
    {
        float angle = i * angleStep;
        float rad = angle * Math::DEG2RAD;

        float cos = std::cos(rad);
        float sin = std::sin(rad);
        Vector3 offset = right * cos + up * sin;
        offset *= 0.05f;

        GameObject* trail = Manager::CreateEmpty(position + offset);
        auto th = trail->AddComponent<PatternTrailHeader>();
        th->Init(m_config, normal, PatternTrailHeader::Axis::Y, angle);

        auto tr = trail->AddComponent<TrailRenderer>(L"Assets/Textures/White.png");
        tr->SetFaceMode(TrailRenderer::TrailFaceMode::FixedDirection);
        tr->SetFixedDirection(normal);
        tr->SetMinSegmentDistance(0.01f);
        tr->SetBlendMode(BlendMode::Additive);

        tr->SetWidthCurve(settings.widthCurve);
        tr->SetTrailLifetime(settings.trailLifeTime);
        tr->SetColorCurve(settings.colorCurve);
        tr->SetWidth(settings.width);


        m_trailHeaders.push_back(trail);
    }

    m_normal = normal;
    m_isCreated = true;
}
