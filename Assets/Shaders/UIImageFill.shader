Shader "UIImageFill"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _UVScale("UV Scale", float2) = (1,1)
        _UVOffset("UV Offset", float2) = (0,0)
        VerticalFill("Vertical Fill", float) = 1.0
        HorizontalFill("Horizontal Fill", float) = 1.0
        VerticalFromTop("Vertical From Top", bool) = true
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

            return false;
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

            return false;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float verticalFill = saturate(VerticalFill);
            const float horizontalFill = saturate(HorizontalFill);

            // 0.0では境界上の補間値だけが残らないよう、先に完全破棄する。
            if (verticalFill <= 0.0f || horizontalFill <= 0.0f)
            {
                clip(-1.0f);
            }

            if (!IsVerticalFilled(input.texCoord.y, verticalFill) ||
                !IsHorizontalFilled(input.texCoord.x, horizontalFill))
            {
                clip(-1.0f);
            }

            const float2 uv = input.texCoord * _UVScale + _UVOffset;
            return _BaseColor * input.color * SpriteTexture.Sample(LinearSampler, uv);
        }
        HLSL_END
    }
}
