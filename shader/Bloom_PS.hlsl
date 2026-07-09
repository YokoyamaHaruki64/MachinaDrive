#include "common.hlsl"

Texture2D srcTexture : register(t0);
Texture2D bloomTexture : register(t1);

SamplerState g_SamplerState : register(s0);

static const float blurRadius = 1.5f;
static const float intensity = 1.5f;

float4 main(PS_IN input) : SV_TARGET
{

    uint width, height;
    bloomTexture.GetDimensions(width, height);


    static const float weights[5] =
    {
        0.227027f,
        0.194595f,
        0.121622f,
        0.054054f,
        0.016216f
    };

    float2 texelSize = 1.0f / float2(width, height);

    float3 source = srcTexture.Sample(
        g_SamplerState,
        input.TexCoord
    ).rgb;

    float3 bloom = 0.0f;

    [unroll]
    for (int y = -4; y <= 4; ++y)
    {
        float2 offset =
            float2(0.0f, y * texelSize.y * blurRadius);

        bloom += bloomTexture.Sample(
            g_SamplerState,
            input.TexCoord + offset
        ).rgb * weights[abs(y)];
    }

    return float4(
        saturate(source + bloom * intensity),
        1.0f
    );
}
