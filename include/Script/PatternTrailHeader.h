#pragma once

#include "System/Component.h"
#include "math/Vector3.h"
#include "math/Curve.h"

class EffectConfig;
class TrailRenderer;

class PatternTrailHeader : public Component
{
public:
    enum Axis
    {
        X,
        Y,
        Z
    };
private:

    EffectConfig* m_config = nullptr;
    TrailRenderer* m_trailRenderer = nullptr;

    Axis m_axis = Axis::Y;
    Vector3 m_startPos;
    Vector3 m_surfaceRight;
    Vector3 m_surfaceUp;

    float m_angle = 0.0f;
    float m_lifeTime = 0.0f;
    float m_elapsedTime = 0.0f;
    float m_trailLifeTime = 0.0f;
    bool m_finished = false;

    void UpdatePosition(float t);
    void SetSurface(const Vector3& normal)
    {
        Vector3::CreateRightUp(normal, m_surfaceRight, m_surfaceUp);
        UpdatePosition(m_elapsedTime / m_lifeTime);
    }

public:
    PatternTrailHeader() = delete;
    explicit PatternTrailHeader(GameObject* owner)
        : Component(owner)
    {
    }
    virtual ~PatternTrailHeader() {}
    virtual void Update();

    void Init(EffectConfig* config, const Vector3& normal, Axis axis, float angle);
};
