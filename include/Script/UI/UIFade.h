#pragma once

#include <string>

#include "Common/Color.h"
#include "System/Core/Script.h"

class UIImage;
class UIText;

// 同一GameObjectのUI描画要素を、進捗に応じて透明化する。
class UIFade final : public Script
{
    GM_SCRIPT(UIFade, "UI Fade")
    USE_FIELDS

    std::string m_easeType = "Linear";
    float m_startAlpha = 1.0f;
    float m_endAlpha = 0.0f;
    bool m_reverse = false;
    bool m_reverseEaseType = true;
    float m_progress = 0.0f;

    UIImage* m_image = nullptr;
    UIText* m_text = nullptr;
    Color m_imageColor{};
    Color m_textColor{};

    void ResolveTargets();
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
