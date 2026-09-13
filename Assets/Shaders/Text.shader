Shader "Text"
{
  Properties
  {
    _BaseColor("Base Color", Color) = (1,1,1,1)
    OutlineColor("Outline Color", Color) = (0,0,0,1)
    OutlineWidth("Outline Width", float) = 0.0
    FontTexelSize("Font Texel Size", float2) = (0.001,0.001)
    _UVScale("UV Scale", float2) = (1,1)
    _UVOffset("UV Offset", float2) = (0,0)
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

    float SampleGlyphAlpha(float2 uv)
    {
      return SpriteTexture.Sample(LinearSampler, uv).a;
    }

    float SampleExpandedAlpha(float2 uv, float width)
    {
      const float2 offset = FontTexelSize * max(width, 0.0f);
      float alpha = SampleGlyphAlpha(uv);
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(-offset.x, -offset.y)));
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(0.0f, -offset.y)));
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(offset.x, -offset.y)));
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(-offset.x, 0.0f)));
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(offset.x, 0.0f)));
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(-offset.x, offset.y)));
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(0.0f, offset.y)));
      alpha = max(alpha, SampleGlyphAlpha(uv + float2(offset.x, offset.y)));
      return alpha;
    }

    float4 PSMain(VSOutput input) : SV_TARGET
    {
      const float2 uv = input.texCoord * _UVScale + _UVOffset;
      const float4 glyphSample = SpriteTexture.Sample(LinearSampler, uv);
      const float glyphAlpha = glyphSample.a;
      const float outlineAlpha = saturate(SampleExpandedAlpha(uv, OutlineWidth) - glyphAlpha) * OutlineColor.a * _BaseColor.a;

      float4 color = float4(OutlineColor.rgb, outlineAlpha);
      color.rgb = lerp(color.rgb, glyphSample.rgb * _BaseColor.rgb, glyphAlpha);
      color.a = max(color.a, glyphAlpha * _BaseColor.a);
      return color * input.color;
    }
    HLSL_END
  }
}
