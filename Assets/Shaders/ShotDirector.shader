Shader "ShotDirector"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _StripeColor("Stripe Color", Color) = (0.2,0.8,1,1)
        _ArrowColor("Arrow Color", Color) = (0.2,0.8,1,1)
        _ScrollSpeed("Scroll Speed", Float) = 1.0
        _ArrowCount("Arrow Count", Float) = 6.0
        _ArrowWidth("Arrow Width", Float) = 0.25
        _ArrowThickness("Arrow Thickness", Float) = 0.06
        _StripeWidth("Stripe Width", Float) = 0.03
        _StripeIntensity("Stripe Intensity", Float) = 0.2
        _EdgeSoftness("Edge Softness", Float) = 0.02
    }

    Pass
    {
        RenderPhase = Transparent
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
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;
            const float4 worldPosition = mul(float4(input.position, 1.0f), World);
            output.position = mul(mul(worldPosition, View), Projection);
            output.color = input.color;
            output.texCoord = input.texCoord;
            return output;
        }

        float ArrowMask(float2 texCoord)
        {
            const float arrowWidth = max(_ArrowWidth, 0.001f);
            const float edgeSoftness = max(_EdgeSoftness, 0.001f);

            const float2 cell = float2(
                texCoord.x - 0.5f,
                frac(texCoord.y * max(_ArrowCount, 1.0f) - Time * _ScrollSpeed) - 0.5f);

            const float distanceFromCenter = abs(cell.x);
            const float arrowLineY = -0.24f + distanceFromCenter * 1.5f;
            const float lineDistance = abs(cell.y - arrowLineY);
            const float lineMask = 1.0f - smoothstep(
                max(_ArrowThickness, 0.001f),
                max(_ArrowThickness, 0.001f) + edgeSoftness,
                lineDistance);
            const float widthMask = 1.0f - smoothstep(
                arrowWidth,
                arrowWidth + edgeSoftness,
                distanceFromCenter);
            return lineMask * widthMask;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float arrowMask = ArrowMask(input.texCoord);
            const float stripeDistance = abs(input.texCoord.x - 0.5f);

            const float edgeSoftness = max(_EdgeSoftness, 0.001f);

            const float stripeMask = 1.0f - smoothstep(
                max(_StripeWidth, 0.001f),
                max(_StripeWidth, 0.001f) + edgeSoftness,
                stripeDistance);

            const float stripeIntensity = stripeMask * _StripeIntensity;
            const float3 rgb =
                _ArrowColor.rgb * arrowMask +
                _StripeColor.rgb * stripeIntensity;
            const float alpha = max(
                _ArrowColor.a * arrowMask,
                _StripeColor.a * stripeIntensity);

            const float4 baseColor = _BaseColor * input.color;
            return float4(rgb * baseColor.rgb, alpha * baseColor.a);
        }
        HLSL_END
    }
}
