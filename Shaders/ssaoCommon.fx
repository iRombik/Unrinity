#include "common.fx"

#define kernelSide cb0.x
#define radius     cb0.y
#define bias       cb0.z

struct VERTEX_OUTPUT
{
    float2 texCoord : TEXCOORD0;
};

struct PIXEL_OUTPUT
{
    float ssaoFactor : SV_TARGET0;
};
