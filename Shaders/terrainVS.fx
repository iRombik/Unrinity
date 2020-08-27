#include "common.fx"
#include "terrainCommon.fx"

[[vk::push_constant]]
struct PUSH_CONSTANT {
    float2 terrainBlockPos;
} pushConstant;

void main(int vertexId : SV_VertexID, out VERTEX_OUTPUT vertexOut, out float4 projPos : SV_Position)
{
    float2 offset = float2((vertexId >> 2) & 3, vertexId & 3) * TERRAIN_VERTEX_OFFSET;
    vertexOut.worldPos = float4(
        pushConstant.terrainBlockPos.x + offset.x, 
        0.f, 
        pushConstant.terrainBlockPos.y + offset.y, 
        1.f);
    projPos = mul(worldViewProj, vertexOut.worldPos);
}