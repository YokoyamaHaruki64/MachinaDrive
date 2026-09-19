


cbuffer PerFrame : register(b0)
{
	matrix View;
	matrix Projection;
	float3 CameraPosition;
	float Time;
	float UnscaledTime;
	float3 PerFramePadding;
}

cbuffer PerObject : register(b1)
{
	matrix World;
	uint ReceiveShadow;
	float3 PerObjectPadding;
}

SamplerState LinearSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);
SamplerState ClampSampler : register(s2);
Texture2DArray ShadowMap : register(t1);

cbuffer ShadowParameters : register(b4)
{
	matrix LightViewProjection[4];
	float2 ShadowMapTexelSize;
	uint CurrentCascadeIndex;
	float ShadowParametersPadding;
	float4 CascadeFarDistances;
}




struct LIGHT
{
	bool Enable;
	bool3 Dummy;
	float4 Direction;
	float4 Diffuse;
	float4 Ambient;
};

cbuffer Lighting : register(b2)
{
	LIGHT Light;
}

struct VS_IN
{
	float4 Position		: POSITION0;
	float4 Normal		: NORMAL0;
	float4 Diffuse		: COLOR0;
	float2 TexCoord		: TEXCOORD0;
};


struct PS_IN
{
	float4 Position		: SV_POSITION;
    float4 Normal		: NORMAL0;
	float4 Diffuse		: COLOR0;
	float2 TexCoord		: TEXCOORD0;
    float4 WorldPos		: POSITION0;
    float4 ObjectNormal	: NORMAL1;
};
