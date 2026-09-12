Shader "GaugeInner"
{
  Properties
  {
    _BaseColor("Base Color", Color) = (1,1,1,1)
    Progress("Progress", float) = 1.0
    SpriteTexture("Sprite Texture", 2D) = ""
    ProgressMask("Progress Mask", Texture2D) = ""
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
      const float progress = saturate(Progress);

      if (progress <= 0.0f)
      {
        clip(-1.0f);
      }

      const float2 uv = input.texCoord * _UVScale + _UVOffset;

      // MaskのR値は実際のゲージ形状に沿った開始0から終了1までの進行度。
      const float maskProgress = ProgressMask.Sample(LinearSampler, input.texCoord).r;
      clip(progress - maskProgress);

      const float4 color = SpriteTexture.Sample(LinearSampler, uv);
      return color * _BaseColor * input.color;
    }
    HLSL_END
  }
}
