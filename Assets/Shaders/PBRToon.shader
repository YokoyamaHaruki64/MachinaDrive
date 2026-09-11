Shader "PBRToon"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1,1,1,1)
        _Emission("Emission", Color) = (0,0,0,1)
        _Metallic("Metallic", Float) = 0
        _Roughness("Roughness", Float) = 0.5
        _ToonBands("Toon Bands", Float) = 4
        _ToonDiffuseStrength("Toon Diffuse Strength", Float) = 1
        _ToonShadowColor("Toon Shadow Color", Color) = (0.35,0.4,0.55,1)
        _ToonSpecularColor("Toon Specular Color", Color) = (1,1,1,1)
        _ToonSpecularThreshold("Toon Specular Threshold", Float) = 0.45
        _RimColor("Rim Color", Color) = (0.2,0.6,1,1)
        _RimPower("Rim Power", Float) = 4
        _RimStrength("Rim Strength", Float) = 0
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
        #include "lighting.hlsl"
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

        float QuantizeToon(float value, float bands)
        {
            const float safeBands = max(floor(bands), 1.0f);
            if (safeBands <= 1.0f)
                return value > 0.0f ? 1.0f : 0.0f;

            return saturate(floor(saturate(value) * safeBands) / (safeBands - 1.0f));
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

            const float shadow = GetShadowAttenuation(input.worldPosition);
            const float toonLight = Light.Enable
                ? QuantizeToon(lighting.lightAmount, _ToonBands)
                : 0.0f;
            const float3 shadowColor = Light.Enable
                ? albedo * _ToonShadowColor.rgb * (1.0f - toonLight)
                : float3(0.0f, 0.0f, 0.0f);
            const float3 diffuse = lighting.diffuseResponse * toonLight * shadow * max(_ToonDiffuseStrength, 0.0f);

            // スペキュラはPBRの応答を閾値化し、細いハイライトとして残す。
            const float3 physicalSpecular = lighting.specularResponse * lighting.lightAmount;
            const float specularStrength = max(
                physicalSpecular.r,
                max(physicalSpecular.g, physicalSpecular.b));
            const float specularMask = Light.Enable
                ? step(_ToonSpecularThreshold, specularStrength)
                : 0.0f;
            const float3 specular = physicalSpecular * specularMask * _ToonSpecularColor.rgb * shadow;

            const float rimFactor = pow(
                saturate(1.0f - lighting.viewAmount),
                max(_RimPower, 1.0f));
            const float3 rim = Light.Enable
                ? _RimColor.rgb * rimFactor * max(_RimStrength, 0.0f)
                : float3(0.0f, 0.0f, 0.0f);

            const float3 color = lighting.ambientResponse + shadowColor + diffuse + specular + rim + _Emission.rgb;
            return float4(color, _BaseColor.a * input.color.a * textureColor.a);
        }
        HLSL_END
    }

    Pass
    {
        RenderPhase = Shadow
        Cull = Front
        Blend = None
        ZWrite = On
        ZTest = LessEqual

        #pragma vertex VSShadow
        #pragma pixel PSShadow
        #pragma shader_model 5.0

        HLSL_BEGIN
        #include "common.hlsl"
        #include "lighting.hlsl"

        struct ShadowVSInput
        {
            float3 position : POSITION0;
            float3 normal : NORMAL0;
        };

        float4 VSShadow(ShadowVSInput input) : SV_POSITION
        {
            float4 worldPosition = mul(float4(input.position, 1.0f), World);
            const float3 worldNormal = normalize(mul(float4(input.normal, 0.0f), World).xyz);
            worldPosition = ApplyShadowNormalBias(worldPosition, worldNormal);
            return GetShadowClipPosition(worldPosition);
        }

        float4 PSShadow() : SV_TARGET
        {
            return 0.0f;
        }
        HLSL_END
    }
}
