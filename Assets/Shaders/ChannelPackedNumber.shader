Shader "ChannelPackedNumber"
{
  Properties
  {
    _BaseColor("Base Color", Color) = (1,1,1,1)
    TextColor("Text Color", Color) = (1,0,0,1)
    GlowColor("Glow Color", Color) = (0,1,0,1)
    OutlineColor("Outline Color", Color) = (0,0,1,1)
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

    float4 PSMain(VSOutput input) : SV_TARGET
    {
      const float2 uv = input.texCoord * _UVScale + _UVOffset;
      const float4 sampleColor = SpriteTexture.Sample(LinearSampler, uv);
      const float3 mask = sampleColor.rgb;
      float4 color =
        TextColor * mask.r +
        GlowColor * mask.g +
        OutlineColor * mask.b;
      color.a = saturate(color.a) * sampleColor.a;
      return color * _BaseColor * input.color;
    }
    HLSL_END
  }
}
