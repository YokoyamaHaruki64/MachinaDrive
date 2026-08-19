Shader "PostProcessBloom"
{
    Properties
    {
        _Threshold("Threshold", float) = 0.8
        _BlurRadius("Blur Radius", float) = 1.0
        _Intensity("Intensity", float) = 2.0
        _Bloom("Bloom Texture", 2D) = ""
    }

    Pass "BloomExtract"
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
            uint width, height;
            PostProcessSource.GetDimensions(width, height);
            const float2 texelSize = 1.0f / float2(width, height);
            const float weights[9] =
            {
                0.1630f, 0.1514f, 0.1205f, 0.0820f, 0.0477f,
                0.0238f, 0.0101f, 0.0037f, 0.0011f
            };

            float3 bloom = 0.0f;
            [unroll]
            for (int x = -8; x <= 8; ++x)
            {
                const float3 color = SamplePostProcessSource(input.texCoord + float2(x * texelSize.x, 0.0f)).rgb;
                const float brightness = max(color.r, max(color.g, color.b));
                bloom += color * smoothstep(_Threshold, 1.0f, brightness) * weights[abs(x)] * _BlurRadius;
            }

            return float4(bloom, 1.0f);
        }
        HLSL_END
    }

    Pass "Composite"
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
            uint width, height;
            _Bloom.GetDimensions(width, height);
            const float2 texelSize = 1.0f / float2(width, height);
            const float weights[9] =
            {
                0.1630f, 0.1514f, 0.1205f, 0.0820f, 0.0477f,
                0.0238f, 0.0101f, 0.0037f, 0.0011f
            };

            float3 bloom = 0.0f;
            [unroll]
            for (int y = -8; y <= 8; ++y)
                bloom += _Bloom.Sample(LinearSampler, input.texCoord + float2(0.0f, y * texelSize.y * _BlurRadius)).rgb * weights[abs(y)];

            return float4(saturate(SamplePostProcessSource(input.texCoord).rgb + bloom * _Intensity), 1.0f);
        }
        HLSL_END
    }
}
