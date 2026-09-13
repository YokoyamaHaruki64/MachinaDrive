Shader "PostProcessHorizontalGlitchTransition"
{
    Properties
    {
        _Progress("Progress", float) = 0
        _MaxOffset("Max Horizontal Offset", float) = 0.12
        _ChromaticOffset("Chromatic Offset", Vector2) = [0.006,0]
        _BandCount("Band Count", int) = 24
        _BandProbability("Band Probability", float) = 0.8
        _NoiseSpeed("Noise Speed", float) = 12
        _BlackoutStart("Blackout Start", float) = 0.72
        _BlackoutSoftness("Blackout Softness", float) = 0.18
        _Seed("Seed", float) = 0
    }

    Pass "HorizontalGlitch"
    {
        RenderPhase = FinalPostProcess
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

        float SmoothProgress(float value)
        {
            value = saturate(value);
            return value * value * (3.0f - 2.0f * value);
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float progress = saturate(_Progress);
            if (progress <= 0.0f)
                return float4(SamplePostProcessSource(input.texCoord).rgb, 1.0f);
            if (progress >= 1.0f)
                return float4(0.0f, 0.0f, 0.0f, 1.0f);

            const int bandCount = min(max(_BandCount, 1), 128);
            const float bandIndex = floor(input.texCoord.y * (float)bandCount);

            const float timeStep = floor(Time * max(_NoiseSpeed, 0.0f));

            const float2 randomSeed = float2(
                bandIndex + _Seed,
                timeStep + bandIndex * 17.0f + _Seed);

            const float bandVisible = step(
                1.0f - saturate(_BandProbability),
                Random(randomSeed));

            const float direction =
                Random(randomSeed + float2(3.1f, 7.2f)) * 2.0f - 1.0f;

            const float offset = direction * max(_MaxOffset, 0.0f) *
                progress * bandVisible;

            const float2 sampleUV = input.texCoord + float2(offset, 0.0f);
            const float2 chromaticOffset = _ChromaticOffset * progress;
            const float2 redUV = sampleUV + chromaticOffset;
            const float2 blueUV = sampleUV - chromaticOffset;

            const float insideRed =
                step(0.0f, redUV.x) * step(redUV.x, 1.0f);
            const float insideGreen =
                step(0.0f, sampleUV.x) * step(sampleUV.x, 1.0f);
            const float insideBlue =
                step(0.0f, blueUV.x) * step(blueUV.x, 1.0f);

            const float3 glitchedSource = float3(
                SamplePostProcessSource(redUV).r * insideRed,
                SamplePostProcessSource(sampleUV).g * insideGreen,
                SamplePostProcessSource(blueUV).b * insideBlue);

            const float blackoutRange = max(_BlackoutSoftness, 0.0001f);

            const float blackout = SmoothProgress(
                (progress - _BlackoutStart) / blackoutRange);

            const float3 result = lerp(glitchedSource, 0.0f, blackout);

            return float4(result, 1.0f);
        }

        HLSL_END
    }
}
