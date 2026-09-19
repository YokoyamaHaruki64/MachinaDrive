
// HUDのフレーム用
Shader "HUDFrameMask"
{
  Properties
  {
    _BaseColor("Base Color", Color) = (1,1,1,1)
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

    float2 NormalizeDirection(float2 direction)
    {
      const float lengthSquared = dot(direction, direction);
      return lengthSquared > 0.0001f ? direction * rsqrt(lengthSquared) : float2(1.0f, 0.0f);
    }

    // 0.0から1.0の範囲でグラデーションの進行度を計算
    float GetGradientProgress(float2 texCoord)
    {
      const float2 direction = NormalizeDirection(GradientDirection);
      const float position = dot(texCoord - 0.5f, direction) + 0.5f;

      const float phase =
        frac(position + GradientOffset + UnscaledTime * GradientSpeed);

      return 1.0f - abs(phase * 2.0f - 1.0f);
    }

    // 0.0から1.0の範囲でハイライトのマスクを計算
    float GetHighlightMask(float2 texCoord)
    {
      const float2 direction = NormalizeDirection(HighlightDirection);
      const float position = dot(texCoord - 0.5f, direction) + 0.5f;

      const float bandPosition =
        frac(position + HighlightOffset + UnscaledTime * HighlightSpeed);

      const float distanceFromBand = abs(bandPosition - 0.5f);

      const float halfWidth = max(HighlightWidth * 0.5f, 0.0001f);

      return 1.0f - smoothstep(halfWidth, halfWidth + 0.01f, distanceFromBand);
    }

    // マスクを取得
    float GetMask(float2 texCoord)
    {
      const float4 maskSample = SpriteTexture.Sample(LinearSampler, texCoord);
      return saturate(UseAlphaMask ? maskSample.a : maskSample.r);
    }

    // ハイライトをぼかす
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

        // 斜め方向のぼかし
      const float diagonal = (
        GetMask(texCoord + blur) * GetHighlightMask(texCoord + blur) +
        GetMask(texCoord + float2(blur.x, -blur.y)) * GetHighlightMask(texCoord + float2(blur.x, -blur.y)) +
        GetMask(texCoord + float2(-blur.x, blur.y)) * GetHighlightMask(texCoord + float2(-blur.x, blur.y)) +
        GetMask(texCoord - blur) * GetHighlightMask(texCoord - blur)) * 0.0625f;

      return center + horizontal + vertical + diagonal;
    }

    float4 PSMain(VSOutput input) : SV_TARGET
    {
      const float mask = GetMask(input.texCoord);

      // グラデーションカラーを計算
      const float4 gradientColor = lerp(
        GradientColorA,
        GradientColorB,
        GetGradientProgress(input.texCoord));

      const float highlight = GetBlurredHighlight(input.texCoord) * HighlightIntensity;

      const float3 rgb =
        gradientColor.rgb *
        _BaseColor.rgb *
        input.color.rgb *
        mask + HighlightColor.rgb * HighlightColor.a * highlight;

      const float baseAlpha = gradientColor.a * _BaseColor.a * input.color.a * mask;

      const float highlightAlpha = HighlightColor.a * input.color.a * highlight;

      const float alpha = max(baseAlpha, highlightAlpha);

      return float4(rgb, alpha);
    }
    HLSL_END
  }
}
