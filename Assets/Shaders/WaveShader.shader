Shader "Wave"
{
    Properties
    {
        _MainTex ("MainTex", 2D) = "white" {}
        _Amplitude1 ("Amplitude1", Float) = 0.13
        _Amplitude2 ("Amplitude2", Float) = 0.23
        _Amplitude3 ("Amplitude3", Float) = 0.31
        _Frequency1 ("Frequency1", Float) = 0.77
        _Frequency2 ("Frequency2", Float) = 1.03
        _Frequency3 ("Frequency3", Float) = 1.31
        _Direction1 ("Direction1", Float2) = (1.0,0.2)
        _Direction2 ("Direction2", Float2) = (-0.4,0.9)
        _Direction3 ("Direction3", Float2) = (0.7,-0.8)
        _Speed1 ("Speed1", Float) = 1.23
        _Speed2 ("Speed2", Float) = 2.0
        _Speed3 ("Speed3", Float) = 0.21
        _TextureWorldScale("Texture World Scale", Float) = 1.0
    }

    RenderPhase = Opaque
    Cull = Back
    Blend = None
    ZWrite = On
    ZTest = LessEqual



    Pass
    {
        #pragma vertex VSMain
        #pragma pixel PSMain
        #pragma shader_model 5.0

        HLSL_BEGIN
        #include "common.hlsl"
        #include "lighting.hlsl"

        struct VSInput
        {
            float3 position : POSITION0;
            // float4 color : COLOR0;
            // float2 texCoord : TEXCOORD0;
        };

        struct VSOutput
        {
            float4 position : SV_POSITION;
            float3 worldPosition : POSITION0;
            float3 normal : NORMAL0;
            float4 color : COLOR0;
            float2 texCoord : TEXCOORD0;
        };

        VSOutput VSMain(VSInput input)
        {
            VSOutput output;

            float3 beforeWorldPos = mul(float4(input.position, 1.0f), World).xyz;

            float2 dir1 = normalize(_Direction1);
            float2 dir2 = normalize(_Direction2);
            float2 dir3 = normalize(_Direction3);

            float phase1 =
                dot(beforeWorldPos.xz, dir1) * _Frequency1
                + _Speed1 * Time;

            float phase2 =
                dot(beforeWorldPos.xz, dir2) * _Frequency2
                + _Speed2 * Time;

            float phase3 =
                dot(beforeWorldPos.xz, dir3) * _Frequency3
                + _Speed3 * Time;

            float s1, c1;
            float s2, c2;
            float s3, c3;

            sincos(phase1, s1, c1);
            sincos(phase2, s2, c2);
            sincos(phase3, s3, c3);

            float waveHeight =
                s1 * _Amplitude1 +
                s2 * _Amplitude2 +
                s3 * _Amplitude3;

            // 傾き
            float slope1 =
                c1 * _Amplitude1 * _Frequency1;

            float slope2 =
                c2 * _Amplitude2 * _Frequency2;

            float slope3 =
                c3 * _Amplitude3 * _Frequency3;

            // XZ平面における傾きベクトル
            float2 gradient =
                dir1 * slope1 +
                dir2 * slope2 +
                dir3 * slope3;

            float3 localPosition = input.position;
            localPosition.y += waveHeight;

            float3 localNormal = normalize(float3(
                -gradient.x,
                1.0f,
                -gradient.y
            ));

            float4 worldPosition = mul(float4(localPosition, 1.0f), World);

            output.worldPosition =
                worldPosition.xyz;

            output.position =
                mul(mul(worldPosition, View), Projection);

            output.normal =
                normalize(mul(localNormal, (float3x3)World));

            output.color = float4(1,1,1,1);
            float texScale = max(_TextureWorldScale, 0.0001f);

            output.texCoord = worldPosition.xz / texScale;

            return output;
        }

        float4 PSMain(VSOutput input) : SV_TARGET
        {
            float3 N = normalize(input.normal);
            float3 L = normalize(-Light.Direction.xyz);

            float shadow = GetShadowAttenuation(input.worldPosition);
            float lightAmount = Light.Enable ?
                saturate((dot(N, L) + 1.0) / 2.0 ) : 1.0f;
            return _MainTex.Sample(LinearSampler, input.texCoord) *
                float4(Light.Ambient.rgb + Light.Diffuse.rgb * lightAmount * shadow, 1.0f);
        }
        HLSL_END
    }
}
