#include "pch.h"

#include "Script/NoisePostProcess.h"

void NoisePostProcess::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
