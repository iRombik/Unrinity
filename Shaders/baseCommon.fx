#include "common.fx"

struct VERTEX_INPUT
{
	float3 position : POSITION;
	float2 texCoord : TEXCOORD0;
	float3 normal   : NORMAL;
};

struct VERTEX_OUTPUT 
{
    float3 worldPos : POSITION;
    float3 worldNormal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct PIXEL_OUTPUT
{
    float4 outColor : SV_Target;
};