#include "Common.hlsl"

Texture2D MainTex : register(t0);

static const float EPSILON = 0.001;

//=========================================================
// Utility
//=========================================================

float Remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    return mad(
        (value - inMin) / (inMax - inMin),
        (outMax - outMin),
        outMin);
}

//=========================================================
// Surface判定
// 最大軸を利用して分岐数削減
//=========================================================

int SurfaceFromNormal(float3 n)
{
    float3 an = abs(n);

    if (an.y > an.x && an.y > an.z)
        return (n.y > 0.0) ? 1 : 2;

    if (an.x > an.z)
        return (n.x > 0.0) ? 3 : 4;

    return (n.z > 0.0) ? 5 : 6;
}

//=========================================================
// Dice UV 
//=========================================================

// 1
float2 GetDice_1(float2 uv)
{
    return uv;
}

// 2
float2 GetDice_2(float2 uv)
{
    bool a = uv.x < 0.5;
    bool b = uv.y < 0.5;

    if (a == b)
    {
        return frac(uv * 2.0);
    }

    return 0.0;
}

// 3
float2 GetDice_3(float2 uv)
{
    const float div = 1.0 / 3.0;

    int ix = (int) (uv.x * 3.0);
    int iy = (int) (uv.y * 3.0);

    if (ix == iy)
    {
        return frac(uv * 3.0);
    }

    return 0.0;
}

// 4
float2 GetDice_4(float2 uv)
{
    return frac(uv * 2.0);
}

// 5
float2 GetDice_5(float2 uv)
{
    float2 tiled = frac(uv * 2.0);

    bool centerX = uv.x >= (1.0 / 3.0) && uv.x < (2.0 / 3.0);
    bool centerY = uv.y >= (1.0 / 3.0) && uv.y < (2.0 / 3.0);

    if (centerX && centerY)
    {
        return frac(uv * 3.0);
    }

    return tiled;
}

// 6
float2 GetDice_6(float2 uv)
{
    const float border = 1.0 / 6.0;

    // 左右白領域
    if (uv.x < border || uv.x > 1.0 - border)
    {
        return 0.0;
    }

    uv.x = Remap(uv.x, border, 1.0 - border, 0.0, 1.0);

    return frac(uv * float2(2.0, 3.0));
}

//=========================================================
// Dice UV Selector
//=========================================================

float2 GetDiceUV(int surfaceID, float2 uv)
{
    switch (surfaceID)
    {
        case 1:
            return GetDice_1(uv);
        case 2:
            return GetDice_6(uv);
        case 3:
            return GetDice_2(uv);
        case 4:
            return GetDice_5(uv);
        case 5:
            return GetDice_3(uv);
        case 6:
            return GetDice_4(uv);
    }

    return 0.0;
}

//=========================================================
// Pixel Shader
//=========================================================

float4 main(PS_IN input) : SV_Target
{
    float3 normal = normalize(input.Normal.xyz);
    float3 objectNormal = normalize(input.ObjectNormal.xyz);

    int surfaceID = SurfaceFromNormal(objectNormal);

    float2 uv = GetDiceUV(surfaceID, input.TexCoord);

    float4 tex = MainTex.Sample(LinearSampler, uv);

    // 1面だけ黒部分を赤
    if (surfaceID == 1 && all(tex.rgb < EPSILON))
    {
        tex = float4(1.0, 0.0, 0.0, 1.0);
    }

    // Half Lambert
    float lightIntensity =
        mad(-dot(normal, Light.Direction.xyz), 0.5, 0.5);

    float3 color =
        input.Diffuse.rgb *
        tex.rgb *
        Light.Diffuse.rgb *
        lightIntensity;

    return float4(color, input.Diffuse.a * tex.a);
}
