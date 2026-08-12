


cbuffer PerFrame : register(b0)
{
	matrix View;
	matrix Projection;
	float3 CameraPosition;
	float Time;
}

cbuffer PerObject : register(b1)
{
	matrix World;
}

SamplerState LinearSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);




struct MATERIAL
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
	float4 Emission;
	float Shininess;
	bool TextureEnable;
	float2 Dummy;
};

#ifndef MATERIAL_PROPERTIES_DEFINED
cbuffer Properties : register(b3)
{
	MATERIAL Material;
}
#endif




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

cbuffer UVBuffer : register(b6)
{
    float2 UVScale;
    float2 UVOffset;
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
