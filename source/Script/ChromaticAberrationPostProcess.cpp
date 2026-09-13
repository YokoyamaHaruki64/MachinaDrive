#include "pch.h"

#include "Script/ChromaticAberrationPostProcess.h"

void ChromaticAberrationPostProcess::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
