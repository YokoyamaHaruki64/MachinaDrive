
#include "common.hlsl"
PS_IN main(in VS_IN input)
{
    PS_IN o;
    o.Position = input.Position;
    o.TexCoord = input.TexCoord;
    o.Diffuse = input.Diffuse;
    return o;
}

