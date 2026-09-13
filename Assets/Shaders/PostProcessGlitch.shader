Shader "PostProcessGlitch"
{
    Properties
    {
        _Intensity("Intensity", float) = 0
        _SliceCount("Slice Count", int) = 48
        _SliceStrength("Slice Strength", float) = 0.015
        _BlockScale("Block Scale", Vector2) = [32,12]
        _BlockStrength("Block Strength", float) = 0.02
        _BlockProbability("Block Probability", float) = 0.3
        _ChromaticOffset("Chromatic Offset", Vector2) = [0.003,0]
        _Speed("Speed", float) = 14
        _Seed("Seed", float) = 0
    }

    Pass "Glitch"
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
            const float2 blockScale = max(_BlockScale, float2(1.0f, 1.0f));
            const float sliceCount = max((float)_SliceCount, 1.0f);
            const float timeStep = floor(Time * max(_Speed, 0.0f));
            const float sliceId = floor(input.texCoord.y * sliceCount);
            const float2 blockId = floor(input.texCoord * blockScale);

            const float sliceNoise =
                Random(float2(sliceId, timeStep) + _Seed) * 2.0f - 1.0f;
            const float blockNoise =
                Random(blockId + timeStep + _Seed) * 2.0f - 1.0f;
            const float blockMask = step(
                1.0f - saturate(_BlockProbability),
                Random(blockId + timeStep * 1.37f + _Seed));

            const float displacement =
                (sliceNoise * _SliceStrength +
                    blockNoise * _BlockStrength * blockMask) * _Intensity;
            const float2 glitchTexCoord =
                input.texCoord + float2(displacement, 0.0f);
            const float2 chromaticOffset =
                _ChromaticOffset * _Intensity * (0.5f + abs(sliceNoise) * 0.5f);

            const float red =
                SamplePostProcessSource(glitchTexCoord + chromaticOffset).r;
            const float green =
                SamplePostProcessSource(glitchTexCoord).g;
            const float blue =
                SamplePostProcessSource(glitchTexCoord - chromaticOffset).b;

            return float4(red, green, blue, 1.0f);
        }

        HLSL_END
    }
}
