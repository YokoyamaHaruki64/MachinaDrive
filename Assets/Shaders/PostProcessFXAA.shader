Shader "PostProcessFXAA"
{
    Properties
    {
        _SubpixelBlending("Subpixel Blending", float) = 0.75
        _EdgeThreshold("Edge Threshold", float) = 0.125
        _EdgeThresholdMin("Edge Threshold Min", float) = 0.0312
    }

    Pass "FXAA"
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

        float _SubpixelBlending;
        float _EdgeThreshold;
        float _EdgeThresholdMin;

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;
            output.position = float4(input.position, 1.0f);
            output.texCoord = input.texCoord;
            return output;
        }

        float Luminance(float3 color)
        {
            return dot(color, float3(0.299f, 0.587f, 0.114f));
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            uint width, height;
            PostProcessSource.GetDimensions(width, height);
            const float2 texelSize = 1.0f / float2(width, height);

            const float3 colorM = SamplePostProcessSource(input.texCoord).rgb;
            const float lumaM = Luminance(colorM);
            const float lumaNW = Luminance(SamplePostProcessSource(input.texCoord + texelSize * float2(-1.0f, -1.0f)).rgb);
            const float lumaNE = Luminance(SamplePostProcessSource(input.texCoord + texelSize * float2(1.0f, -1.0f)).rgb);
            const float lumaSW = Luminance(SamplePostProcessSource(input.texCoord + texelSize * float2(-1.0f, 1.0f)).rgb);
            const float lumaSE = Luminance(SamplePostProcessSource(input.texCoord + texelSize * float2(1.0f, 1.0f)).rgb);
            const float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
            const float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

            if (lumaMax - lumaMin < max(_EdgeThresholdMin, lumaMax * _EdgeThreshold))
                return float4(colorM, 1.0f);

            float2 direction;
            direction.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
            direction.y = (lumaNW + lumaSW) - (lumaNE + lumaSE);

            const float directionReduce = max(
                (lumaNW + lumaNE + lumaSW + lumaSE) * (_SubpixelBlending * 0.25f),
                1.0f / 128.0f);
            const float reciprocalDirectionMinimum = 1.0f / (min(abs(direction.x), abs(direction.y)) + directionReduce);
            direction = clamp(direction * reciprocalDirectionMinimum, -8.0f, 8.0f) * texelSize;

            const float3 colorA = 0.5f * (
                SamplePostProcessSource(input.texCoord + direction * (1.0f / 3.0f - 0.5f)).rgb +
                SamplePostProcessSource(input.texCoord + direction * (2.0f / 3.0f - 0.5f)).rgb);
            const float3 colorB = colorA * 0.5f + 0.25f * (
                SamplePostProcessSource(input.texCoord - direction * 0.5f).rgb +
                SamplePostProcessSource(input.texCoord + direction * 0.5f).rgb);
            const float lumaB = Luminance(colorB);

            return float4(lumaB < lumaMin || lumaB > lumaMax ? colorA : colorB, 1.0f);
        }
        HLSL_END
    }
}
