#include "pch.h"

#include "System/GM_Time.h"

#include "Script/PatternTrailHeader.h"
#include "Script/EffectConfig.h"

#include "Component/TrailRenderer.h"
#include "Component/Transform.h"

void PatternTrailHeader::Init(EffectConfig* config, const Vector3& normal, Axis axis, float angle)
{
    m_startPos = m_owner->GetTransform()->GetPosition();
    auto settings = config->GetPatternTrailConfig();
    m_lifeTime = 1.0f / (settings.speed == 0.0f ? 0.01f : settings.speed); // 0除算回避
    m_config = config;
    m_axis = axis;
    m_angle = angle;
    SetSurface(normal);
}

void PatternTrailHeader::Update()
{
    m_elapsedTime += Time::GetDeltaTime();
    if(m_elapsedTime >= m_lifeTime)
    {
        m_owner->Destroy();
    }

    // 経過時間を正規化
    float t = m_elapsedTime / m_lifeTime;

    UpdatePosition(t);
}


void PatternTrailHeader::UpdatePosition(float t)
{
    // 現在の設定を取得
    auto& settings = m_config->GetPatternTrailConfig();

    // カーブから位置を取得(二次元のみ)
    Vector3 localPos = settings.positionCurve.Evaluate(1.0f - t);
    localPos.z = 0.0f; // Z軸は無視

    // ローカル位置をワールド位置に変換
    float rad = m_angle * Math::DEG2RAD;
    float cosAngle = std::cos(rad);
    float sinAngle = std::sin(rad);
    Vector3 curvePos = Vector3(
        localPos.x * cosAngle - localPos.y * sinAngle,
        localPos.x * sinAngle + localPos.y * cosAngle,
        0.0f
    );

    curvePos *= settings.length; // 長さを適用

    // ワールド位置を計算
    Vector3 worldPos = m_startPos +
        m_surfaceRight * curvePos.x +
        m_surfaceUp * curvePos.y;

    transform()->SetPosition(worldPos);

}

