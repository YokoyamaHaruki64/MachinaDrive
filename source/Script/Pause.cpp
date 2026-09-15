#include "pch.h"
#include "Component/ComponentInc.h"

#include "Script/Pause.h"

bool Pause::IsPause()
{
    return s_isPause;
}

void Pause::SetPause(bool isPause)
{
    if (s_isPause == isPause)
    {
        return;
    }

    if (isPause)
    {
        s_timeScaleBeforePause = Time::GetTimeScale();
        Time::SetTimeScale(0.0f);
    }
    else
    {
        Time::SetTimeScale(s_timeScaleBeforePause);
    }

    s_isPause = isPause;
}
