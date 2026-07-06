#pragma once

#include <array>
#include "math/MathCommon.h"

template<typename T>
class Curve
{
private:
    std::array<T, 4> m_points{};

public:
    Curve() = default;

    Curve(const T& startValue, const T& endValue)
        : m_points{
            startValue,
            Math::Lerp(startValue, endValue, 1.0f / 3.0f),
            Math::Lerp(startValue, endValue, 2.0f / 3.0f),
            endValue
        }
    {
    }

    Curve(const std::array<T, 4>& points)
        : m_points(points)
    {
    }

    ~Curve() = default;

    void SetControlPoints(const std::array<T, 4>& points)
    {
        m_points = points;
    }

    const std::array<T, 4>& GetControlPoints() const
    {
        return m_points;
    }

    std::array<T, 4> GetControlPointsCopy() const
    {
        return m_points;
    }

    void SetStartValue(const T& startValue) { m_points[0] = startValue; }
    void SetControlValue1(const T& controlValue1) { m_points[1] = controlValue1; }
    void SetControlValue2(const T& controlValue2) { m_points[2] = controlValue2; }
    void SetEndValue(const T& endValue) { m_points[3] = endValue; }

    const T& GetStartValue() const { return m_points[0]; }
    const T& GetControlValue1() const { return m_points[1]; }
    const T& GetControlValue2() const { return m_points[2]; }
    const T& GetEndValue() const { return m_points[3]; }

    T Evaluate(float t) const
    {
        t = Math::Saturate(t);

        const float u = 1.0f - t;

        return m_points[0] * (u * u * u)
            + m_points[1] * (3.0f * u * u * t)
            + m_points[2] * (3.0f * u * t * t)
            + m_points[3] * (t * t * t);
    }
};
