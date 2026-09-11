#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

float4 GetShadowClipPosition(float4 worldPosition)
{
    return mul(worldPosition, LightViewProjection);
}

float4 ApplyShadowNormalBias(float4 worldPosition, float3 worldNormal)
{
    const float3 lightDirection = normalize(-Light.Direction.xyz);
    const float normalBias = 0.0002f * (1.0f - saturate(dot(normalize(worldNormal), lightDirection)));
    worldPosition.xyz += normalize(worldNormal) * normalBias;
    return worldPosition;
}

float GetShadowAttenuation(float4 worldPosition)
{
    if (ReceiveShadow == 0u || !Light.Enable)
        return 1.0f;

    const float4 shadowPosition = GetShadowClipPosition(worldPosition);
    const float3 projectedPosition = shadowPosition.xyz / shadowPosition.w;
    const float2 shadowUV = projectedPosition.xy * float2(0.5f, -0.5f) + 0.5f;
    if (any(shadowUV < 0.0f) || any(shadowUV > 1.0f) || projectedPosition.z < 0.0f || projectedPosition.z > 1.0f)
        return 1.0f;

    float visibility = 0.0f;
    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            const float2 sampleUV = shadowUV + float2(x, y) * ShadowMapTexelSize * 0.35f;
            visibility += ShadowMap.SampleCmpLevelZero(ShadowSampler, sampleUV, projectedPosition.z - 0.000005f);
        }
    }
    return visibility / 25.0f;
}

#endif
