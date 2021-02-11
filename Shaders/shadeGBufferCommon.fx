#include "common.fx"

struct VERTEX_OUTPUT 
{
    float2 texCoord : TEXCOORD0;
};

struct PIXEL_OUTPUT
{
    float4 color : SV_Target;
};

