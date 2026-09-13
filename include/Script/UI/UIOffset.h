#pragma once

#include <string>

#include "math/Vector2.h"
#include "System/Core/Script.h"

class UITransform;

// 進捗0から1へ、指定したUIオフセットまで移動する。
class UIOffset final : public Script
{
    GM_SCRIPT(UIOffset, "UI Offset")
    USE_FIELDS

    std::string m_easeType = "Linear";
    Vector2 m_offset{};
    bool m_reverse = false;
    bool m_reverseEaseType = true;
    float m_progress = 0.0f;

    UITransform* m_transform = nullptr;
    Vector2 m_baseOffset{};

    void ResolveTarget();
    void Apply();

public:
    void OnStart() override;
    void SetProgress(float progress);
    float GetProgress() const { return m_progress; }
    void SetReverse(bool reverse);
    bool IsReverse() const { return m_reverse; }
    void SetReverseEaseType(bool enabled);
    bool IsReverseEaseType() const { return m_reverseEaseType; }
};
