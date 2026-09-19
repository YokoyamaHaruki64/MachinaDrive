#pragma once

#include "System/Core/Script.h"

// ゲーム中のポーズ状態を共有するためのScript。
// 通常の時間は停止し、ポーズ中にも動かす処理はUnscaled時間を使用する。
class Pause final : public Script
{
    GM_SCRIPT_OPTION(Pause, "Pause", false, true)

public:
    static bool IsPause();
    static void SetPause(bool isPause);

private:
    inline static bool s_isPause{ false };
    inline static float s_timeScaleBeforePause{ 1.0f };
};
