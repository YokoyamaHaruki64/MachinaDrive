Shader "PostProcessGrayScale"
{
    Properties
    {
        _Intensity("Intensity", float) = 0
    }
    Pass "GrayScale"
    {
        RenderPhase = PostProcess
        Cull = None
        Blend = None
        ZWrite = Off
        ZTest = None

        #pragma vertex VSMain
        #pragma pixel PSMain
        #pragma shader_model 5.0

        HLSL_BEGIN
        #include "common.hlsl"
        #include "postprocess_common.hlsl"

        struct VSInput
        {
            float3 position : POSITION0;
            float2 texCoord : TEXCOORD0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float2 texCoord : TEXCOORD0;
        };

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;
            output.position = float4(input.position, 1.0f);
            output.texCoord = input.texCoord;
            return output;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float4 source = SamplePostProcessSource(input.texCoord);
            const float luminance = dot(source.rgb, float3(0.2126f, 0.7152f, 0.0722f));
            const float3 color = lerp(source.rgb, luminance.xxx, saturate(_Intensity));
            return float4(color, source.a);
        }
        HLSL_END
    }
}
