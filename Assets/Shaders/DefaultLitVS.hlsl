
#include "Common.hlsl"

PS_IN main(VS_IN input)
{
    PS_IN output;

    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);
    output.Position = mul(input.Position, wvp);

    // ���s�ړ������������ă��[���h�s���@���ɓK�p
    output.Normal = float4(normalize(mul(input.Normal.xyz, (float3x3) World)), 0.0);
    output.TexCoord = input.TexCoord;
    output.Diffuse = input.Diffuse;
    //output.worldPosition = mul(input.Position, World);
    return output;
}
