Shader "ParticleInstancedAlpha"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _UVScale("UV Scale", float2) = (1,1)
        _UVOffset("UV Offset", float2) = (0,0)
        _MainTex("Main Texture", 2D) = "Assets/Local/Textures/particle.png"
    }

    Pass
    {
        RenderPhase = Transparent
        Cull = None
        Blend = AlphaBlend
        ZWrite = Off
        ZTest = LessEqual
        Instanced = true

        #pragma vertex VSMain
        #pragma pixel PSMain
        #pragma shader_model 5.0

        HLSL_BEGIN
        #include "common.hlsl"

        struct ObjectInput
        {
            float4x4 world;
            float4 color;
            float4 emissionColor;
            float4 uvTransform;
        };

        StructuredBuffer<ObjectInput> ObjectInputs : register(t2);

        struct VSInput
        {
            float3 position : POSITION0;
            float2 texCoord : TEXCOORD0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float4 color : COLOR0;
            float4 emissionColor : COLOR1;
            float2 texCoord : TEXCOORD0;
        };

        VSOutput VSMain(VSInput input, uint instanceID : SV_InstanceID)
        {
            const ObjectInput particle = ObjectInputs[instanceID];
            VSOutput output;
            const float4 worldPosition = mul(float4(input.position, 1.0f), particle.world);
            output.position = mul(mul(worldPosition, View), Projection);
            output.color = particle.color;
            output.emissionColor = particle.emissionColor;
            output.texCoord = input.texCoord * (particle.uvTransform.xy * _UVScale) +
                particle.uvTransform.zw + _UVOffset;
            return output;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float4 sampled = _MainTex.Sample(LinearSampler, input.texCoord);
            float4 color = _BaseColor * input.color * sampled;
            color.rgb += input.emissionColor.rgb * sampled.rgb;
            return color;
        }
        HLSL_END
    }
}
