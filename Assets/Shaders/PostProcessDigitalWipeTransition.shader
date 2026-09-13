Shader "PostProcessDigitalWipeTransition"
{
    Properties
    {
        _Progress("Progress", float) = 0
        _Direction("Direction", Vector2) = [1,0]
        _TransitionColor("Transition Color", Color) = (0,0,0,1)
        _BlockScale("Block Scale", Vector2) = [32,12]
        _Jitter("Block Jitter", float) = 0.025
        _NoiseSpeed("Noise Speed", float) = 12
        _EdgeWidth("Edge Width", float) = 0.04
        _EdgeColor("Edge Color", Color) = (0.1,0.8,1,1)
        _EdgeIntensity("Edge Intensity", float) = 1
        _ChromaticOffset("Chromatic Offset", Vector2) = [0.003,0]
        _Seed("Seed", float) = 0
    }

    Pass "DigitalWipe"
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
            const float3 source = SamplePostProcessSource(input.texCoord).rgb;
            if (_Progress <= 0.0f)
                return float4(source, 1.0f);
            if (_Progress >= 1.0f)
                return float4(_TransitionColor.rgb, 1.0f);

            const float2 blockScale = max(_BlockScale, float2(1.0f, 1.0f));
            const float2 direction = _Direction;
            const float directionLength = max(length(direction), 0.0001f);
            const float2 normalizedDirection = direction / directionLength;
            const float minProjection =
                min(0.0f, normalizedDirection.x) + min(0.0f, normalizedDirection.y);
            const float maxProjection =
                max(0.0f, normalizedDirection.x) + max(0.0f, normalizedDirection.y);
            const float projectionRange = max(maxProjection - minProjection, 0.0001f);
            const float coordinate = saturate(
                (dot(input.texCoord, normalizedDirection) - minProjection) / projectionRange);

            const float timeStep = floor(Time * max(_NoiseSpeed, 0.0f));
            const float2 blockId = floor(input.texCoord * blockScale);
            const float blockNoise =
                Random(blockId + timeStep + _Seed) * 2.0f - 1.0f;
            const float edgePosition = saturate(
                _Progress + blockNoise * max(_Jitter, 0.0f));
            const float edgeWidth = max(_EdgeWidth, 0.0001f);
            const float uncovered = smoothstep(
                edgePosition - edgeWidth,
                edgePosition + edgeWidth,
                coordinate);
            const float coverage = 1.0f - uncovered;
            const float edge = 1.0f - smoothstep(
                0.0f,
                edgeWidth,
                abs(coordinate - edgePosition));

            const float2 chromaticOffset =
                _ChromaticOffset * edge * (1.0f - coverage);
            const float3 chromaticSource = float3(
                SamplePostProcessSource(input.texCoord + chromaticOffset).r,
                source.g,
                SamplePostProcessSource(input.texCoord - chromaticOffset).b);
            const float3 result = lerp(chromaticSource, _TransitionColor.rgb, coverage);
            return float4(
                result + _EdgeColor.rgb * edge * _EdgeIntensity,
                1.0f);
        }

        HLSL_END
    }
}
