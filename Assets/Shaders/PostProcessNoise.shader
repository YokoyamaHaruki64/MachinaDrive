Shader "PostProcessNoise"
{
    Properties
    {
        _Intensity("Intensity", float) = 0
        _Scale("Scale", Vector2) = [320,180]
        _Speed("Speed", Vector2) = [0,12]
        _Seed("Seed", float) = 0
    }
    Pass "Noise"
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

        float Random(float2 value)
        {
            return frac(sin(dot(value, float2(12.9898f, 78.233f))) * 43758.5453f);
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float2 noisePosition =
                floor(input.texCoord * _Scale) + _Speed * UnscaledTime + _Seed;

            const float noise =
                Random(noisePosition) * 2.0f - 1.0f;
            return float4(saturate(SamplePostProcessSource(input.texCoord).rgb + noise * _Intensity), 1.0f);
        }
        HLSL_END
    }
}
