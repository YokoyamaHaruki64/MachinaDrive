#pragma once

#include "math/MathCommon.h"

#include <cmath>
#include <string_view>

enum class EaseType
{
    Linear,
    InSine,
    OutSine,
    InOutSine,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic,
};

// 0.0から1.0までの進捗を、指定した補間曲線へ変換する。
class Easing final
{
public:
    static float Linear(float progress)
    {
        return Clamp(progress);
    }

    static float InSine(float progress)
    {
        return 1.0f - std::cos(Clamp(progress) * Math::PI_DIV2);
    }

    static float OutSine(float progress)
    {
        return std::sin(Clamp(progress) * Math::PI_DIV2);
    }

    static float InOutSine(float progress)
    {
        return -(std::cos(Math::PI * Clamp(progress)) - 1.0f) * 0.5f;
    }

    static float InQuad(float progress)
    {
        const float value = Clamp(progress);
        return value * value;
    }

    static float OutQuad(float progress)
    {
        const float value = Clamp(progress);
        return 1.0f - (1.0f - value) * (1.0f - value);
    }

    static float InOutQuad(float progress)
    {
        const float value = Clamp(progress);
        return value < 0.5f
            ? 2.0f * value * value
            : 1.0f - std::pow(-2.0f * value + 2.0f, 2.0f) * 0.5f;
    }

    static float InCubic(float progress)
    {
        const float value = Clamp(progress);
        return value * value * value;
    }

    static float OutCubic(float progress)
    {
        const float value = Clamp(progress);
        return 1.0f - std::pow(1.0f - value, 3.0f);
    }

    static float InOutCubic(float progress)
    {
        const float value = Clamp(progress);
        return value < 0.5f
            ? 4.0f * value * value * value
            : 1.0f - std::pow(-2.0f * value + 2.0f, 3.0f) * 0.5f;
    }

    static float Evaluate(float progress, EaseType type)
    {
        switch (type)
        {
        case EaseType::InSine: return InSine(progress);
        case EaseType::OutSine: return OutSine(progress);
        case EaseType::InOutSine: return InOutSine(progress);
        case EaseType::InQuad: return InQuad(progress);
        case EaseType::OutQuad: return OutQuad(progress);
        case EaseType::InOutQuad: return InOutQuad(progress);
        case EaseType::InCubic: return InCubic(progress);
        case EaseType::OutCubic: return OutCubic(progress);
        case EaseType::InOutCubic: return InOutCubic(progress);
        case EaseType::Linear:
        default:
            return Linear(progress);
        }
    }

    static EaseType ToEaseType(std::string_view value)
    {
        if (value == "InSine") return EaseType::InSine;
        if (value == "OutSine") return EaseType::OutSine;
        if (value == "InOutSine") return EaseType::InOutSine;
        if (value == "InQuad") return EaseType::InQuad;
        if (value == "OutQuad") return EaseType::OutQuad;
        if (value == "InOutQuad") return EaseType::InOutQuad;
        if (value == "InCubic") return EaseType::InCubic;
        if (value == "OutCubic") return EaseType::OutCubic;
        if (value == "InOutCubic") return EaseType::InOutCubic;
        return EaseType::Linear;
    }

    static EaseType Reverse(EaseType type)
    {
        switch (type)
        {
        case EaseType::InSine: return EaseType::OutSine;
        case EaseType::OutSine: return EaseType::InSine;
        case EaseType::InQuad: return EaseType::OutQuad;
        case EaseType::OutQuad: return EaseType::InQuad;
        case EaseType::InCubic: return EaseType::OutCubic;
        case EaseType::OutCubic: return EaseType::InCubic;
        default: return type;
        }
    }

    static const char* ToString(EaseType type)
    {
        switch (type)
        {
        case EaseType::InSine: return "InSine";
        case EaseType::OutSine: return "OutSine";
        case EaseType::InOutSine: return "InOutSine";
        case EaseType::InQuad: return "InQuad";
        case EaseType::OutQuad: return "OutQuad";
        case EaseType::InOutQuad: return "InOutQuad";
        case EaseType::InCubic: return "InCubic";
        case EaseType::OutCubic: return "OutCubic";
        case EaseType::InOutCubic: return "InOutCubic";
        case EaseType::Linear:
        default:
            return "Linear";
        }
    }

private:
    static float Clamp(float progress)
    {
        return std::isfinite(progress) ? Math::Clamp(progress, 0.0f, 1.0f) : 0.0f;
    }
};
