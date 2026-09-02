#pragma once

#include "math/Vector2.h"

// UIの導出済み矩形を、Canvas座標とRenderTargetのpx座標で共用する。
struct UIRect
{
    Vector2 min{};
    Vector2 max{};

    Vector2 GetSize() const { return max - min; }
    Vector2 GetCenter() const { return (min + max) * 0.5f; }

    bool Contains(const Vector2& point) const
    {
        return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
    }

    bool Overlaps(const UIRect& other) const
    {
        return min.x < other.max.x && max.x > other.min.x &&
            min.y < other.max.y && max.y > other.min.y;
    }
};
