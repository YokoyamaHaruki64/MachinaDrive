#ifndef LIGHTING_HLSL
#define LIGHTING_HLSL

float4 GetShadowClipPosition(float4 worldPosition)
{
    return mul(worldPosition, LightViewProjection[CurrentCascadeIndex]);
}

float4 ApplyShadowNormalBias(float4 worldPosition, float3 worldNormal)
{
    const float3 lightDirection = normalize(-Light.Direction.xyz);
    const float normalBias = 0.0002f * (1.0f - saturate(dot(normalize(worldNormal), lightDirection)));
    worldPosition.xyz += normalize(worldNormal) * normalBias;
    return worldPosition;
}

uint GetShadowCascadeIndex(float viewDepth)
{
    if (viewDepth < CascadeFarDistances.x)
        return 0u;
    if (viewDepth < CascadeFarDistances.y)
        return 1u;
    if (viewDepth < CascadeFarDistances.z)
        return 2u;
    return 3u;
}

float SampleShadowCascade(float4 worldPosition, uint cascadeIndex)
{
    const float4 shadowPosition = mul(worldPosition, LightViewProjection[cascadeIndex]);
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
            visibility += ShadowMap.SampleCmpLevelZero(
                ShadowSampler, float3(sampleUV, cascadeIndex), projectedPosition.z - 0.000005f);
        }
    }
    return visibility / 25.0f;
}

float GetShadowAttenuation(float4 worldPosition)
{
    if (ReceiveShadow == 0u || !Light.Enable)
        return 1.0f;

    const float viewDepth = mul(worldPosition, View).z;
    if (viewDepth > CascadeFarDistances.w)
        return 1.0f;

    const uint cascadeIndex = GetShadowCascadeIndex(viewDepth);
    const float attenuation = SampleShadowCascade(worldPosition, cascadeIndex);
    if (cascadeIndex >= 3u)
        return attenuation;

    const float cascadeNearDistance = cascadeIndex == 0u ? 0.1f : CascadeFarDistances[cascadeIndex - 1u];
    const float cascadeFarDistance = CascadeFarDistances[cascadeIndex];
    const float blendStart = lerp(cascadeNearDistance, cascadeFarDistance, 0.9f);
    const float blendAmount = saturate((viewDepth - blendStart) / (cascadeFarDistance - blendStart));
    return lerp(attenuation, SampleShadowCascade(worldPosition, cascadeIndex + 1u), blendAmount);
}

#endif
