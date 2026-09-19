#include "pch.h"

#include "Script/GlitchPostProcess.h"

void GlitchPostProcess::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
