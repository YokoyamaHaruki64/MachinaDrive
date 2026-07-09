
#include "common.hlsl"

Texture2D srcTexture : register(t0);
SamplerState g_SamplerState : register(s0);

static const float threshold = 0.8f;
static const float blurRadius = 1.0f;

float3 ExtractBright(float3 color)
{
    float brightness = max(color.r, max(color.g, color.b));
    return color * smoothstep(threshold, 1.0f, brightness);
}

float4 main(PS_IN input) : SV_TARGET
{
    uint width, height;
    srcTexture.GetDimensions(width, height);
    float2 texelSize = 1.0f / float2(width, height);

    static const float weights[9] =
    {
        0.1630f,
        0.1514f,
        0.1205f,
        0.0820f,
        0.0477f,
        0.0238f,
        0.0101f,
        0.0037f,
        0.0011f
    };

    float3 bloom = 0;

    [unroll]
    for (int x = -8; x <= 8; ++x)
    {
        float2 uv = input.TexCoord +
        float2(x * texelSize.x, 0);

        bloom += ExtractBright(
        srcTexture.Sample(g_SamplerState, uv).rgb
        ) * weights[abs(x)];
    }
    return float4(bloom, 1.0f);
}

