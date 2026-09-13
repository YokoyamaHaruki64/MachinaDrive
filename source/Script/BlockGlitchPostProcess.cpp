#include "pch.h"

#include "Script/BlockGlitchPostProcess.h"

void BlockGlitchPostProcess::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
