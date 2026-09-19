#include "pch.h"

#include "Script/RadialBlurPostProcess.h"

void RadialBlurPostProcess::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
