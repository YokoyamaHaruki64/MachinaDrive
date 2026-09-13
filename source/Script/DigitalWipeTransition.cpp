#include "pch.h"

#include "Script/DigitalWipeTransition.h"

void DigitalWipeTransition::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
