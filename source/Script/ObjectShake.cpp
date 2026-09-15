#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/ObjectShake.h"

void ObjectShake::OnStart()
{
    m_appliedOffset = Vector3::Zero();
    m_smoothedOffset = Vector3::Zero();
    m_elapsedTime = 0.0f;
    m_duration = 0.0f;
    m_power = 0.0f;
    m_isShaking = false;
}

void ObjectShake::Shake(float duration, float power)
{
    m_elapsedTime = 0.0f;
    m_duration = Math::Max(duration, 0.0f);
    m_power = Math::Max(power, 0.0f);
    m_isShaking = m_duration > 0.0f;
}

void ObjectShake::LateUpdate()
{
    Transform* targetTransform = transform();
    if (!targetTransform)
    {
        return;
    }

    // 他のAnimation Scriptが更新した位置から、前フレームの揺れだけを取り除く。
    const Vector3 baseLocalPosition = targetTransform->GetLocalPosition() - m_appliedOffset;
    if (!m_isShaking)
    {
        if (m_appliedOffset.LengthSqr() > 0.0f)
        {
            targetTransform->SetLocalPosition(baseLocalPosition);
            m_appliedOffset = Vector3::Zero();
        }
        return;
    }

    m_elapsedTime += Time::GetDeltaTime();
    if (m_elapsedTime >= m_duration)
    {
        m_isShaking = false;
        m_smoothedOffset = Vector3::Zero();
        targetTransform->SetLocalPosition(baseLocalPosition);
        m_appliedOffset = Vector3::Zero();
        return;
    }

    const float progress = Math::Clamp(m_elapsedTime / m_duration, 0.0f, 1.0f);
    const float falloff = 1.0f - progress;
    const Vector3 randomOffset = Random::InsideUnitSphere();
    m_smoothedOffset = Math::Lerp(m_smoothedOffset, randomOffset, 0.2f);
    m_appliedOffset = m_smoothedOffset * (m_power * falloff);

    targetTransform->SetLocalPosition(baseLocalPosition + m_appliedOffset);
}
