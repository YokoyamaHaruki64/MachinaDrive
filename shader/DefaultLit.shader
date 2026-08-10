Shader "DefaultLit"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _Emission("Emission", Color) = (0,0,0,1)
        _Shininess("Shininess", Float) = 32
        _MainTex("Main Texture", 2D) = ""
    }

    Pass
    {
        RenderPhase = Opaque
        Cull = Back
        Blend = None
        ZWrite = On
        ZTest = LessEqual

        #pragma vertex VSMain
        #pragma pixel PSMain
        #pragma shader_model 5.0

        HLSL_BEGIN
        #include "shader/common.hlsl"

        SamplerState MainSampler : register(s16);

        struct VSInput
        {
            float3 position : POSITION0;
            float3 normal : NORMAL0;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float3 normal : NORMAL0;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;
            const float4 worldPosition = mul(float4(input.position, 1.0f), World);
            output.position = mul(mul(worldPosition, View), Projection);
            output.normal = normalize(mul(float4(input.normal, 0.0f), World).xyz);
            output.color = input.color;
            output.texCoord = input.texCoord;
            return output;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float3 normal = normalize(input.normal);
            const float lightAmount = Light.Enable ? saturate(dot(normal, -Light.Direction.xyz)) : 1.0f;
            const float4 textureColor = _MainTex.Sample(MainSampler, input.texCoord);
            const float3 lighting = Light.Ambient.rgb + Light.Diffuse.rgb * lightAmount;
            return float4((_BaseColor.rgb * input.color.rgb * textureColor.rgb * lighting) + _Emission.rgb,
                _BaseColor.a * input.color.a * textureColor.a);
        }
        HLSL_END
    }
}
