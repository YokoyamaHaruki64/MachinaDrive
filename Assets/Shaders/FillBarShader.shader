Shader "FillBarShader"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _UVScale("UV Scale", float2) = (1,1)
        _UVOffset("UV Offset", float2) = (0,0)
        GradientColorA("Gradient Color A", Color) = (0.15,0.8,1,1)
        GradientColorB("Gradient Color B", Color) = (0.6,1,0.35,1)
        GradientDirection("Gradient Direction", float2) = (1,0)
        GradientOffset("Gradient Offset", float) = 0.0
        GradientSpeed("Gradient Speed", float) = 0.0
        HighlightColor("Highlight Color", Color) = (1,1,1,1)
        HighlightIntensity("Highlight Intensity", float) = 0.75
        HighlightWidth("Highlight Width", float) = 0.04
        HighlightBlur("Highlight Blur", float) = 0.015
        HighlightDirection("Highlight Direction", float2) = (1,1)
        HighlightOffset("Highlight Offset", float) = 0.0
        HighlightSpeed("Highlight Speed", float) = 0.0
        UseAlphaMask("Use Alpha Mask", bool) = true
        Fill("Fill", float) = 1.0
        Chip("Chip", float) = 0.0
        BarColor("Bar Color", Color) = (1,1,1,1)
        ChipColor("Chip Color", Color) = (1,0.2,0.2,1)
        VerticalFromTop("Vertical From Top", bool) = false
        VerticalFromCenter("Vertical From Center", bool) = false
        VerticalFromBottom("Vertical From Bottom", bool) = false
        HorizontalFromLeft("Horizontal From Left", bool) = true
        HorizontalFromCenter("Horizontal From Center", bool) = false
        HorizontalFromRight("Horizontal From Right", bool) = false
    }

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

        Texture2D SpriteTexture : register(t2);

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

        bool IsVerticalFilled(float texCoordY, float fill)
        {
            if (VerticalFromTop)
            {
                return texCoordY >= 1.0f - fill;
            }

            if (VerticalFromCenter)
            {
                return abs(texCoordY - 0.5f) <= fill * 0.5f;
            }

            if (VerticalFromBottom)
            {
                return texCoordY <= fill;
            }

            return true;
        }

        bool IsHorizontalFilled(float texCoordX, float fill)
        {
            if (HorizontalFromLeft)
            {
                return texCoordX <= fill;
            }

            if (HorizontalFromCenter)
            {
                return abs(texCoordX - 0.5f) <= fill * 0.5f;
            }

            if (HorizontalFromRight)
            {
                return texCoordX >= 1.0f - fill;
            }

            return true;
        }

        bool IsFilled(float2 texCoord, float fill)
        {
            if (VerticalFromTop || VerticalFromCenter || VerticalFromBottom)
            {
                return IsVerticalFilled(texCoord.y, fill);
            }

            if (HorizontalFromLeft || HorizontalFromCenter || HorizontalFromRight)
            {
                return IsHorizontalFilled(texCoord.x, fill);
            }

            return false;
        }

        float2 NormalizeDirection(float2 direction)
        {
            const float lengthSquared = dot(direction, direction);
            return lengthSquared > 0.0001f ? direction * rsqrt(lengthSquared) : float2(1.0f, 0.0f);
        }

        float GetGradientProgress(float2 texCoord)
        {
            const float2 direction = NormalizeDirection(GradientDirection);
            const float position = dot(texCoord - 0.5f, direction) + 0.5f;
            const float phase = frac(position + GradientOffset + UnscaledTime * GradientSpeed);
            return 1.0f - abs(phase * 2.0f - 1.0f);
        }

        float GetHighlightMask(float2 texCoord)
        {
            const float2 direction = NormalizeDirection(HighlightDirection);
            const float position = dot(texCoord - 0.5f, direction) + 0.5f;
            const float bandPosition = frac(position + HighlightOffset + UnscaledTime * HighlightSpeed);
            const float distanceFromBand = abs(bandPosition - 0.5f);
            const float halfWidth = max(HighlightWidth * 0.5f, 0.0001f);
            return 1.0f - smoothstep(halfWidth, halfWidth + 0.01f, distanceFromBand);
        }

        float GetMask(float2 texCoord)
        {
            const float2 uv = texCoord * _UVScale + _UVOffset;
            const float4 maskSample = SpriteTexture.Sample(LinearSampler, uv);
            return saturate(UseAlphaMask ? maskSample.a : maskSample.r);
        }

        float GetBlurredHighlight(float2 texCoord)
        {
            const float2 blur = float2(max(HighlightBlur, 0.0f), max(HighlightBlur, 0.0f));
            const float center = GetMask(texCoord) * GetHighlightMask(texCoord) * 0.25f;
            const float horizontal = (
                GetMask(texCoord + float2(blur.x, 0.0f)) * GetHighlightMask(texCoord + float2(blur.x, 0.0f)) +
                GetMask(texCoord - float2(blur.x, 0.0f)) * GetHighlightMask(texCoord - float2(blur.x, 0.0f))) * 0.125f;
            const float vertical = (
                GetMask(texCoord + float2(0.0f, blur.y)) * GetHighlightMask(texCoord + float2(0.0f, blur.y)) +
                GetMask(texCoord - float2(0.0f, blur.y)) * GetHighlightMask(texCoord - float2(0.0f, blur.y))) * 0.125f;
            const float diagonal = (
                GetMask(texCoord + blur) * GetHighlightMask(texCoord + blur) +
                GetMask(texCoord + float2(blur.x, -blur.y)) * GetHighlightMask(texCoord + float2(blur.x, -blur.y)) +
                GetMask(texCoord + float2(-blur.x, blur.y)) * GetHighlightMask(texCoord + float2(-blur.x, blur.y)) +
                GetMask(texCoord - blur) * GetHighlightMask(texCoord - blur)) * 0.0625f;
            return center + horizontal + vertical + diagonal;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float fill = saturate(Fill);
            const float chip = saturate(Chip);
            const float chipFill = saturate(fill + chip);

            // バーで塗りつぶされているかどうかを判定
            const bool isBarFilled =
                fill > 0.0f && IsFilled(input.texCoord, fill);

            // チップで塗りつぶされているかどうかを判定
            const bool isChipFilled =
                chip > 0.0f && IsFilled(input.texCoord, chipFill);

            // バーとチップの両方で塗りつぶされていない場合はピクセルを破棄
            if (!isBarFilled && !isChipFilled)
            {
                clip(-1.0f);
            }

            const float2 uv = input.texCoord * _UVScale + _UVOffset;
            const float4 textureSample = SpriteTexture.Sample(LinearSampler, uv);
            const float4 fillColor = isBarFilled ? BarColor : ChipColor;
            const float4 gradientColor = lerp(
                GradientColorA,
                GradientColorB,
                GetGradientProgress(input.texCoord));
            const float highlight = GetBlurredHighlight(input.texCoord) * HighlightIntensity;
            const float3 rgb = gradientColor.rgb * fillColor.rgb * _BaseColor.rgb * input.color.rgb * textureSample.rgb
                + HighlightColor.rgb * HighlightColor.a * highlight;
            const float baseAlpha = gradientColor.a * fillColor.a * _BaseColor.a * input.color.a
                * (UseAlphaMask ? textureSample.a : textureSample.r);
            const float highlightAlpha = HighlightColor.a * input.color.a * highlight;
            return float4(rgb, max(baseAlpha, highlightAlpha));
        }
        HLSL_END
    }
}
