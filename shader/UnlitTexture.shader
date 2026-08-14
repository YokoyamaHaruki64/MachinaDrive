Shader "UnlitTexture"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _UVScale("UV Scale", Vector2) = (1,1)
        _UVOffset("UV Offset", Vector2) = (0,0)
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
        #include "common.hlsl"

        struct VSInput
        {
            float3 position : POSITION0;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;
            const float4 worldPosition = mul(float4(input.position, 1.0f), World);
            output.position = mul(mul(worldPosition, View), Projection);
            output.color = input.color;
            output.texCoord = input.texCoord * _UVScale + _UVOffset;
            return output;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            return _BaseColor * input.color * _MainTex.Sample(LinearSampler, input.texCoord);
        }
        HLSL_END
    }
}
