#include "pch.h"

#include "Script/GrayScalePostProcess.h"

void GrayScalePostProcess::Apply()
{
    if (GetMaterial())
    {
        ExecuteScreenPassAndSwap(0);
    }
}

void GrayScalePostProcess::SetIntensity(float intensity)
{
    m_intensity = Math::Clamp(intensity, 0.0f, 1.0f);
    GetImpulseOverride().SetFloat("_Intensity", m_intensity);
}
