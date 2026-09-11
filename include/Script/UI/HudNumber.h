#pragma once

#include "System/Core/Script.h"

// 子UIImageを左から桁として扱う。RGBマスクの色合成は後続で実装する。
class HudNumber final : public Script
{
    GM_SCRIPT(HudNumber, "HUD Number")
    USE_FIELDS

    int m_value = 0;
    void ApplyDigits();

public:
    void OnStart() override;
    void SetValue(int value);
    int GetValue() const { return m_value; }
};
