#define M_PI 3.1415926535897932384626433832795

struct POINT_LIGHT {
    float3 pos;
    float  attenuationParam;
    float3 color;
    float  intensity;
    float3 ambient;
};

[[vk::binding(0)]]
cbuffer commonBuffer : register(b0) 
{
    float3   viewPos;
    float    curTime;
    float4x4 worldViewProj;
};

[[vk::binding(1)]]
cbuffer lightBuffer : register (b1) {
    POINT_LIGHT light0;
};

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