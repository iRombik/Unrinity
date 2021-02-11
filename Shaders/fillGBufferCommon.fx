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

struct GBUFFER_OUTPUT
{
	float4 albedo   : SV_TARGET0;
	float4 normal   : SV_TARGET1;
	float4 metalnessRoughness : SV_TARGET2;
	float4 worldPos : SV_TARGET3;
};
