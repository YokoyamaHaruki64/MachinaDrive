#include "common.hlsl"

Texture2D srcTexture : register(t0);
Texture2D bloomTexture : register(t1);


static const float blurRadius = 1.0f;
static const float intensity = 2.0f;

float4 main(PS_IN input) : SV_TARGET
{

    uint width, height;
    bloomTexture.GetDimensions(width, height);

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

    float2 texelSize = 1.0f / float2(width, height);

    float3 source = srcTexture.Sample(
        LinearSampler,
        input.TexCoord
    ).rgb;

    float3 bloom = 0.0f;

    [unroll]
    for (int y = -8; y <= 8; ++y)
    {
        float2 offset =
            float2(0.0f, y * texelSize.y * blurRadius);

        bloom += bloomTexture.Sample(
            LinearSampler,
            input.TexCoord + offset
        ).rgb * weights[abs(y)];
    }

    return float4(
        saturate(source + bloom * intensity),
        1.0f
    );
}
