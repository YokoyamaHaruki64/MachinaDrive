#include "Common.hlsl"

Texture2D MainTex : register(t0);
SamplerState MainSampler : register(s0);



float4 main(PS_IN input) : SV_Target
{
    // ハーフランバートモデルを利用してライト計算 
    float3 normal = normalize(input.Normal.xyz);
    float lightIntensity = 0.5 + -dot(normal, Light.Direction.xyz) * 0.5;
    float2 uv = input.TexCoord;
    uv.x += sin(uv.y * 10.0 * 6.28318) * 0.2;

    
    float4 tex = MainTex.Sample(MainSampler, uv);
    float3 returnColor = 
    input.Diffuse.rgb * 
    tex.rgb *
    Light.Diffuse.rgb * 
    lightIntensity;
    return float4(returnColor, input.Diffuse.a * tex.a);
}

int SurfaceFromNormal(float3 normal)
{
    // 法線からサーフェイスIDを計算する
    // ここでは単純に法線の方向に基づいてIDを割り当てる例を示す
    if (normal.y > 0.5) return 1; // 上向きの面
    if (normal.y < -0.5) return 2; // 下向きの面
    if (normal.x > 0.5) return 3; // 右向きの面
    if (normal.x < -0.5) return 4; // 左向きの面
    if (normal.z > 0.5) return 5; // 前向きの面
    if (normal.z < -0.5) return 6; // 後ろ向きの面
    return 0; // その他の面
}