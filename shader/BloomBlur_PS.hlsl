
#include "common.hlsl"

Texture2D srcTexture : register(t0);
SamplerState g_SamplerState : register(s0);

static const float threshold = 0.9f;
static const float blurRadius = 1.5f;

float3 ExtractBright(float3 color)
{
    float brightness = max(color.r, max(color.g, color.b));
    return color * smoothstep(threshold, 1.0f, brightness);
}

float4 main(PS_IN input) : SV_TARGET
{
    uint width, height;
    srcTexture.GetDimensions(width, height);

    static const float weights[5] =
    {
        0.227027f,
        0.194595f,
        0.121622f,
        0.054054f,
        0.016216f
    };

    float2 texelSize = 1.0f / float2(width, height);
    float3 bloom = 0.0f;

    [unroll]
    for (int x = -4; x <= 4; ++x)
    {
        float2 offset =
            float2(x * texelSize.x * blurRadius, 0.0f);

        float3 color = srcTexture.Sample(
            g_SamplerState,
            input.TexCoord + offset
        ).rgb;

        bloom += ExtractBright(color) * weights[abs(x)];
    }

    return float4(bloom, 1.0f);
}

