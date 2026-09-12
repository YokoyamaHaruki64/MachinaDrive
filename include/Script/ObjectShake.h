#pragma once

#include "System/Core/Script.h"

// 所有GameObjectのローカル位置へ、球内のランダムな揺れを加える再利用可能なScript。
class ObjectShake final : public Script
{
    GM_SCRIPT(ObjectShake, "Object Shake")

private:
    Vector3 m_appliedOffset = Vector3::Zero();
    Vector3 m_smoothedOffset = Vector3::Zero();
    float m_elapsedTime = 0.0f;
    float m_duration = 0.0f;
    float m_power = 0.0f;
    bool m_isShaking = false;

public:
    void OnStart() override;
    void LateUpdate() override;

    void Shake(float duration, float power);
    bool IsShaking() const { return m_isShaking; }
};
