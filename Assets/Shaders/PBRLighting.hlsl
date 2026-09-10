#ifndef PBR_LIGHTING_INCLUDED
#define PBR_LIGHTING_INCLUDED

static const float PBR_PI = 3.14159265359f;
static const float PBR_INV_PI = 0.31830988618f;

struct PBRLightingResult
{
    // Toon側で拡散光を量子化できるよう、応答色と光量を分離して返す。
    float3 diffuseResponse;
    float3 specularResponse;
    float3 ambientResponse;
    float lightAmount;
    float viewAmount;
};

float3 PBRSafeNormalize(float3 value)
{
    return value * rsqrt(max(dot(value, value), 1.0e-6f));
}

float PBRDistributionGGX(float normalHalf, float roughness)
{
    const float alpha = roughness * roughness;
    const float alphaSquared = alpha * alpha;
    const float denominator = (normalHalf * normalHalf) * (alphaSquared - 1.0f) + 1.0f;
    return alphaSquared / max(PBR_PI * denominator * denominator, 1.0e-5f);
}

float PBRGeometrySchlickGGX(float normalDirection, float roughness)
{
    const float k = (roughness + 1.0f) * (roughness + 1.0f) * 0.125f;
    return normalDirection / max(normalDirection * (1.0f - k) + k, 1.0e-5f);
}

float PBRGeometrySmith(float normalView, float normalLight, float roughness)
{
    return PBRGeometrySchlickGGX(normalView, roughness) *
        PBRGeometrySchlickGGX(normalLight, roughness);
}

float3 PBRFresnelSchlick(float viewHalf, float3 baseReflectance)
{
    const float factor = pow(1.0f - saturate(viewHalf), 5.0f);
    return baseReflectance + (1.0f - baseReflectance) * factor;
}

PBRLightingResult EvaluatePBRLighting(
    float3 albedo,
    float3 normal,
    float3 viewDirection,
    float3 lightDirection,
    float3 lightColor,
    float3 ambientColor,
    float metallic,
    float roughness)
{
    PBRLightingResult result;

    const float3 surfaceNormal = PBRSafeNormalize(normal);
    const float3 surfaceView = PBRSafeNormalize(viewDirection);
    const float3 surfaceLight = PBRSafeNormalize(lightDirection);
    const float3 halfDirection = PBRSafeNormalize(surfaceView + surfaceLight);

    const float normalLight = saturate(dot(surfaceNormal, surfaceLight));
    const float normalView = saturate(dot(surfaceNormal, surfaceView));
    const float normalHalf = saturate(dot(surfaceNormal, halfDirection));
    const float viewHalf = saturate(dot(surfaceView, halfDirection));

    const float clampedMetallic = saturate(metallic);
    const float clampedRoughness = clamp(roughness, 0.045f, 1.0f);
    const float3 baseReflectance = lerp(
        float3(0.04f, 0.04f, 0.04f),
        albedo,
        clampedMetallic);
    const float3 fresnel = PBRFresnelSchlick(viewHalf, baseReflectance);
    const float distribution = PBRDistributionGGX(normalHalf, clampedRoughness);
    const float geometry = PBRGeometrySmith(normalView, normalLight, clampedRoughness);
    const float3 specular = (distribution * geometry * fresnel) /
        max(4.0f * normalView * normalLight, 1.0e-4f);
    const float3 diffuseWeight = (1.0f - fresnel) * (1.0f - clampedMetallic);

    result.diffuseResponse = diffuseWeight * albedo * PBR_INV_PI * lightColor;
    result.specularResponse = specular * lightColor;
    result.ambientResponse = diffuseWeight * albedo * ambientColor;
    result.lightAmount = normalLight;
    result.viewAmount = normalView;
    return result;
}

#endif
