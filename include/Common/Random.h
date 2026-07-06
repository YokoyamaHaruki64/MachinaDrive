#pragma once
#include <random>
#include <algorithm>
#include <cmath>

#include "math/Vector3.h"

class Random
{
private:
    inline static std::mt19937& Engine()
    {
        static std::random_device rd;
        static std::mt19937 engine(rd());
        return engine;
    }

public:
    inline static float Range(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(Engine());
    }

    inline static int Range(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(Engine());
    }

    inline static Vector3 InsideUnitSphere()
    {
        const int MAX_ATTEMPTS = 32; // 最大試行回数
        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
        {
            Vector3 v(
                Range(-1.0f, 1.0f),
                Range(-1.0f, 1.0f),
                Range(-1.0f, 1.0f)
            );

            if (v.LengthSqr() <= 1.0f)
                return v;
        }
        return Vector3::Up(); // 最大試行回数を超えた場合は上方向の単位ベクトルを返す
    }

    inline static Vector3 OnUnitSphere()
    {
        Vector3 v = InsideUnitSphere();

        if (v.LengthSqr() <= 0.0001f) // 長さが0に近い場合は上方向の単位ベクトルを返す
            return Vector3::Up();

        return v.Normalized();
    }
};
