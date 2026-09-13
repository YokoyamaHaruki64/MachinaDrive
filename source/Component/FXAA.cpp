#include "pch.h"

#include "Component/FXAA.h"

FXAA::FXAA(GameObject* owner)
    : PostProcess(owner)
{
}

void FXAA::Apply()
{
    if (GetMaterial())
        ExecuteScreenPassAndSwap(0);
}
