#include "Common.hlsl"

Texture2D MainTex : register(t0);



float4 main(PS_IN input) : SV_Target
{
    // ハーフランバートモデルを利用してライト計算 
    float3 normal = normalize(input.Normal.xyz);
    float lightIntensity = 0.5 + -dot(normal, Light.Direction.xyz) * 0.5;
    float2 uv = input.TexCoord;
    uv.x += sin(uv.y * 10.0 * 6.28318) * 0.2;

    
    float4 tex = MainTex.Sample(LinearSampler, uv);
    float3 returnColor = 
    input.Diffuse.rgb * 
    tex.rgb *
    Light.Diffuse.rgb * 
    lightIntensity;
    return float4(returnColor, input.Diffuse.a * tex.a);
}
