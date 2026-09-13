Shader "PostProcessChromaticAberration"
{
    Properties
    {
        _Intensity("Intensity", float) = 0
        _RedOffset("Red Offset", Vector2) = [0.003,0]
        _GreenOffset("Green Offset", Vector2) = [0,0]
        _BlueOffset("Blue Offset", Vector2) = [-0.003,0]
    }
    Pass "ChromaticAberration"
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
            return float4(
                SamplePostProcessSource(input.texCoord + _RedOffset * _Intensity).r,
                SamplePostProcessSource(input.texCoord + _GreenOffset * _Intensity).g,
                SamplePostProcessSource(input.texCoord + _BlueOffset * _Intensity).b,
                1.0f);
        }
        HLSL_END
    }
}
