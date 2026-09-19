Shader "PostProcessBlockGlitch"
{
    Properties
    {
        _Intensity("Intensity", float) = 0
        _BlockCount("Block Count", int) = 8
        _BlockSizeMin("Block Size Min", Vector2) = [0.02,0.01]
        _BlockSizeMax("Block Size Max", Vector2) = [0.16,0.05]
        _Probability("Probability", float) = 0.35
        _Speed("Speed", float) = 10
        _Seed("Seed", float) = 0
    }

    Pass "BlockGlitch"
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
            const int blockCount = min(max(_BlockCount, 0), 32);
            const float timeStep = floor(UnscaledTime * max(_Speed, 0.0f));
            const float2 minSize = min(max(_BlockSizeMin, 0.0f), max(_BlockSizeMax, 0.0f));
            const float2 maxSize = max(max(_BlockSizeMin, 0.0f), max(_BlockSizeMax, 0.0f));
            float blockMask = 0.0f;

            [loop]
            for (int blockIndex = 0; blockIndex < 32; ++blockIndex)
            {
                if (blockIndex >= blockCount)
                    break;

                const float blockSeed = (float)blockIndex + _Seed;
                const float2 randomSeed = float2(
                    blockSeed,
                    timeStep + blockSeed * 17.0f);
                const float visible = step(
                    1.0f - saturate(_Probability),
                    Random(randomSeed));
                const float2 center = float2(
                    Random(randomSeed + float2(3.1f, 7.2f)),
                    Random(randomSeed + float2(9.4f, 1.6f)));
                const float2 size = lerp(
                    minSize,
                    maxSize,
                    Random(randomSeed + float2(5.7f, 11.3f)));
                const float2 blockMin = center - size * 0.5f;
                const float2 blockMax = center + size * 0.5f;
                const float inside =
                    step(blockMin.x, input.texCoord.x) *
                    step(blockMin.y, input.texCoord.y) *
                    step(input.texCoord.x, blockMax.x) *
                    step(input.texCoord.y, blockMax.y);

                blockMask = max(blockMask, inside * visible);
            }

            const float blackness = saturate(blockMask * _Intensity);
            const float3 source = SamplePostProcessSource(input.texCoord).rgb;
            return float4(lerp(source, 0.0f, blackness), 1.0f);
        }

        HLSL_END
    }
}
