#include "pch.h"

#include "System/GM_Time.h"

#include "Script/PatternTrailHeader.h"
#include "Component/Transform.h"


void PatternTrailHeader::OnStart()
{
    m_startPos = m_owner->GetTransform()->GetPosition();
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
    // 時間そのものをカーブで補間
    t = m_timeCurve.Evaluate(t);

    UpdatePosition(t);
}

void PatternTrailHeader::UpdatePosition(float t)
{
    // カーブから位置を取得(二次元のみ)
    Vector3 localPos = m_positionCurve.Evaluate(1.0f - t);
    localPos.z = 0.0f; // Z軸は無視

    // ローカル位置をワールド位置に変換
    float rad = angle * Math::DEG2RAD;
    float cosAngle = std::cos(rad);
    float sinAngle = std::sin(rad);
    Vector3 curvePos = Vector3(
        localPos.x * cosAngle - localPos.y * sinAngle,
        localPos.x * sinAngle + localPos.y * cosAngle,
        0.0f
    );

    Vector3 worldPos = m_startPos +
        m_surfaceRight * curvePos.x +
        m_surfaceUp * curvePos.y;

    transform()->SetPosition(worldPos);

}

