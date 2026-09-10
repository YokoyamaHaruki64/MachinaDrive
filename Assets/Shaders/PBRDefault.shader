Shader "PBRDefault"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _Emission("Emission", Color) = (0,0,0,1)
        _Metallic("Metallic", Float) = 0
        _Roughness("Roughness", Float) = 0.5
        _MainTex("Main Texture", 2D) = ""
    }

    Pass
    {
        RenderPhase = Opaque
        Cull = Back
        Blend = None
        ZWrite = On
        ZTest = LessEqual

        #pragma vertex VSMain
        #pragma pixel PSMain
        #pragma shader_model 5.0

        HLSL_BEGIN
        #include "common.hlsl"
        #include "PBRLighting.hlsl"

        struct VSInput
        {
            float3 position : POSITION0;
            float3 normal : NORMAL0;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float3 worldPosition : TEXCOORD1;
            float3 normal : NORMAL0;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;
            const float4 worldPosition = mul(float4(input.position, 1.0f), World);
            output.position = mul(mul(worldPosition, View), Projection);
            output.worldPosition = worldPosition.xyz;
            output.normal = PBRSafeNormalize(mul(float4(input.normal, 0.0f), World).xyz);
            output.color = input.color;
            output.texCoord = input.texCoord;
            return output;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            const float4 textureColor = _MainTex.Sample(LinearSampler, input.texCoord);
            const float3 albedo = saturate(_BaseColor.rgb * input.color.rgb * textureColor.rgb);
            const float3 normal = PBRSafeNormalize(input.normal);
            const float3 viewDirection = PBRSafeNormalize(CameraPosition - input.worldPosition);
            const float3 lightDirection = PBRSafeNormalize(-Light.Direction.xyz);
            const float3 lightColor = Light.Enable ? Light.Diffuse.rgb : float3(0.0f, 0.0f, 0.0f);

            const PBRLightingResult lighting = EvaluatePBRLighting(
                albedo,
                normal,
                viewDirection,
                lightDirection,
                lightColor,
                Light.Ambient.rgb,
                _Metallic,
                _Roughness);
            const float3 directLighting =
                (lighting.diffuseResponse + lighting.specularResponse) * lighting.lightAmount;
            const float3 color = lighting.ambientResponse + directLighting + _Emission.rgb;

            return float4(color, _BaseColor.a * input.color.a * textureColor.a);
        }
        HLSL_END
    }
}
