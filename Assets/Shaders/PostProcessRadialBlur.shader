Shader "PostProcessRadialBlur"
{
    Properties
    {
        _Intensity("Intensity", float) = 0
        _Center("Center", Vector2) = [0.5,0.5]
        _SampleCount("Sample Count", int) = 8
    }

    Pass "RadialBlur"
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
            const float2 direction = input.texCoord - _Center;
            float3 color = 0.0f;

            const int sampleCount = clamp(_SampleCount, 1, 16);

            [loop]
            for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
            {
                const float progress =
                    sampleCount == 1 ? 0.0f : (float)sampleIndex / (sampleCount - 1);

                color += SamplePostProcessSource(input.texCoord - direction * progress * _Intensity).rgb;
            }

            return float4(color / sampleCount, 1.0f);
        }
        HLSL_END
    }
}
