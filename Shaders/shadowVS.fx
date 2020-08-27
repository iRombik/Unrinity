#include "common.fx"

struct VERTEX_INPUT
{
    float3 position : POSITION;
};

[[vk::push_constant]]
struct PUSH_CONSTANT {
    float4x4 modelMatrix;
} pushConstant;

void main(in VERTEX_INPUT vertexIn, out float4 projPos : SV_Position) {
    float4 worldPos = mul(pushConstant.modelMatrix, float4(vertexIn.position, 1.0f));
    projPos = mul(dirLightViewProj, worldPos);
}