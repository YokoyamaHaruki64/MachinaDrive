#include "Common.hlsl"

Texture2D MainTex : register(t0);
SamplerState MainSampler : register(s0);


float Remap(
    float value,
    float inMin,
    float inMax,
    float outMin,
    float outMax)
{
    return outMin +
        (value - inMin) *
        (outMax - outMin) /
        (inMax - inMin);
}

int SurfaceFromNormal(float3 normal)
{
    // 法線からサーフェイスIDを計算する
    // ここでは単純に法線の方向に基づいてIDを割り当てる例を示す
    if (normal.y > 0.5)
        return 1; // 上向きの面
    if (normal.y < -0.5)
        return 2; // 下向きの面
    if (normal.x > 0.5)
        return 3; // 右向きの面
    if (normal.x < -0.5)
        return 4; // 左向きの面
    if (normal.z > 0.5)
        return 5; // 前向きの面
    if (normal.z < -0.5)
        return 6; // 後ろ向きの面
    return 0; // その他の面
}


// 1の面のUVを計算する関数
float2 GetDice_1(float2 inputUV)
{
    return inputUV;
}

float2 GetDice_2(float2 inputUV)
{
    if (inputUV.x < 0.5 && inputUV.y < 0.5)
    {
        return inputUV * 2.0; // 0-0.5の範囲を0-1に拡大
    }
    else if (inputUV.x >= 0.5 && inputUV.y >= 0.5)
    {
        return (inputUV - float2(0.5, 0.5)) * 2.0; // 0.5-1の範囲を0-1に拡大
    }
    else
    {
        return float2(0.0, 0.0);
    }
}

// 3の面のUVを計算する関数
float2 GetDice_3(float2 inputUV)
{
    // 左上から斜め
    
    if (inputUV.x < 0.33 && inputUV.y < 0.33)
    {
        return inputUV * 3.0;
    }
    else if (inputUV.x >= 0.33 && inputUV.x < 0.66 && inputUV.y >= 0.33 && inputUV.y < 0.66)
    {
        return (inputUV - float2(0.33, 0.33)) * 3.0;
    }
    else if (inputUV.x >= 0.66 && inputUV.y >= 0.66)
    {
        return (inputUV - float2(0.66, 0.66)) * 3.0;
    }
    else
    {
        return float2(0.0, 0.0);
    }
}

float2 GetDice_4(float2 inputUV)
{
    return inputUV * 2.0; // 0～2でタイリング
}

float2 GetDice_5(float2 inputUV)
{
    // 4の面にさらに中央のみ追加
    float2 uv = GetDice_4(inputUV);
    
    if (inputUV.x >= 0.33 && inputUV.x < 0.66 && inputUV.y >= 0.33 && inputUV.y < 0.66)
    {
        return (inputUV - float2(0.33, 0.33)) * 3.0;
    }
    else
    {
        return uv;
    }
    
}

float2 GetDice_6(float2 inputUV)
{
    const float whiteRange = 1.0 / 6.0;
    if (inputUV.x < whiteRange || inputUV.x > 1.0 - whiteRange)
    {
        return float2(0.0, 0.0); // 左右は白
    }
    else
    {
        float2 uv = inputUV;
        uv.x = Remap(uv.x, whiteRange, 1.0 - whiteRange, 0.0, 1.0); // 白い部分を除いてUVをリマップ
        
        if (uv.x < 0.5)
        {
            if (uv.y < 0.33)
            {
                return uv * float2(2.0, 3.0); // 左上
            }
            else if (uv.y >= 0.33 && uv.y < 0.66)
            {
                return (uv - float2(0.0, 0.33)) * float2(2.0, 3.0); // 左中
            }
            else
            {
                return (uv - float2(0.0, 0.66)) * float2(2.0, 3.0); // 左下
            }
        }
        else
        {
            if (uv.y < 0.33)
            {
                return (uv - float2(0.5, 0.0)) * float2(2.0, 3.0); // 右上
            }
            else if (uv.y >= 0.33 && uv.y < 0.66)
            {
                return (uv - float2(0.5, 0.33)) * float2(2.0, 3.0); // 右中
            }
            else
            {
                return (uv - float2(0.5, 0.66)) * float2(2.0, 3.0); // 右下
            }
        }
    }

}

float2 GetDiceUV(float3 normal, float2 inputUV)
{
    int surfaceID = SurfaceFromNormal(normal);
    
    switch (surfaceID)
    {
        case 1:
            return GetDice_1(inputUV);
        case 2:
            return GetDice_6(inputUV);
        case 3:
            return GetDice_2(inputUV);
        case 4:
            return GetDice_5(inputUV);
        case 5:
            return GetDice_3(inputUV);
        case 6:
            return GetDice_4(inputUV);
        default:
            return float2(0.0, 0.0);
    }
}

float4 main(PS_IN input) : SV_Target
{
    // ハーフランバートモデルを利用してライト計算
    float3 normal = normalize(input.Normal.xyz);
    float lightIntensity = 0.5 + -dot(normal, Light.Direction.xyz) * 0.5;
    float3 objectNormal = normalize(input.ObjectNormal.xyz);
    float2 uv = GetDiceUV(objectNormal, input.TexCoord);
    
    float4 tex = MainTex.Sample(MainSampler, uv);
    if (tex.x < 0.001 && tex.y < 0.001 && tex.z < 0.001 && SurfaceFromNormal(objectNormal) == 1)
    {
        tex = float4(1.0, 0.0, 0.0, 1.0); // 黒い部分は白にする
    }

    float3 returnColor =
    input.Diffuse.rgb *
    tex.rgb *
    Light.Diffuse.rgb *
    lightIntensity;
    
    
    return float4(returnColor, input.Diffuse.a * tex.a);
}