
#include "common.hlsl"


Texture2D		g_Texture : register(t0);


void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{

	if (Material.TextureEnable)
	{
        float2 uv = In.TexCoord * UVScale + UVOffset;
        outDiffuse = g_Texture.Sample(LinearSampler, uv);
		outDiffuse *= In.Diffuse;
	}
	else
	{
		outDiffuse = In.Diffuse;
	}
	
}
