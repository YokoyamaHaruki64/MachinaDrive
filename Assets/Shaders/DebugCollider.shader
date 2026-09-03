Shader "DebugCollider"
{
    Pass
    {
        RenderPhase = Overlay
        Cull = None
        Blend = AlphaBlend
        ZWrite = Off
        ZTest = LessEqual

        #pragma vertex VSMain
        #pragma pixel PSMain
        #pragma shader_model 5.0

        HLSL_BEGIN
        #include "common.hlsl"

        struct VSInput
        {
            float3 position : POSITION0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
        };

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;
            const float4 worldPosition = mul(float4(input.position, 1.0f), World);
            output.position = mul(mul(worldPosition, View), Projection);
            return output;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            return float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        HLSL_END
    }
}
