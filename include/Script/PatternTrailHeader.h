#pragma once

#include "System/Component.h"
#include "math/Vector3.h"
#include "math/Curve.h"

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
    CurveVector3 m_positionCurve = CurveVector3(
    {
        {0.0f,0.0f,0.0f,0.3f},
        {1.0f,1.0f,1.0f,0.0f}
    },
    {
        {0.0f,0.0f,0.0f,0.0f},
        {1.0f,1.0f,1.0f,0.0f}
    },
    {
        {0.0f,0.0f,0.0f,1.0f},
        {0.0f,1.0f,0.0f,1.0f}
    });


    Axis m_axis = Axis::Y;
    Vector3 m_startPos;
    Vector3 m_surfaceRight;
    Vector3 m_surfaceUp;

    float angle = 0.0f;
    float m_lifeTime = 1.0f;
    float m_elapsedTime = 0.0f;

    void UpdatePosition(float t);

public:
    PatternTrailHeader() = delete;
    explicit PatternTrailHeader(GameObject* owner, Axis axis)
        : Component(owner), m_axis(axis)
    {
    }
    virtual ~PatternTrailHeader() {}
    virtual void OnStart();
    virtual void Update();

    void SetAxis(Axis axis){ m_axis = axis; }
    void SetLifeTime(float lifeTime) { m_lifeTime = lifeTime; }
    void SetPositionCurve(const CurveVector3& positionCurve) { m_positionCurve = positionCurve; }
    void SetAngle(float angle) { this->angle = angle; }
    void SetSurface(const Vector3& normal)
    {
        Vector3::CreateRightUp(normal, m_surfaceRight, m_surfaceUp);
        UpdatePosition(m_elapsedTime / m_lifeTime);
    }

};
