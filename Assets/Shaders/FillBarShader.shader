Shader "FillBarShader"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _UVScale("UV Scale", float2) = (1,1)
        _UVOffset("UV Offset", float2) = (0,0)
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
            const float4 fillColor = isBarFilled ? BarColor : ChipColor;
            return _BaseColor * input.color * fillColor * SpriteTexture.Sample(LinearSampler, uv);
        }
        HLSL_END
    }
}
