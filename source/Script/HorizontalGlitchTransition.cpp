#include "pch.h"

#include "Script/HorizontalGlitchTransition.h"

void HorizontalGlitchTransition::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
