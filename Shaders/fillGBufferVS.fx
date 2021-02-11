#include "fillGBufferCommon.fx"

[[vk::push_constant]]
struct PUSH_CONSTANT {
    float4x4 modelMatrix;
} pushConstant;

void main(in VERTEX_INPUT vertexIn, out float4 projPos : SV_Position, out VERTEX_OUTPUT vertexOut) {
    float4 worldPos = mul(pushConstant.modelMatrix, float4(vertexIn.position, 1.0f));
    projPos = mul(worldViewProj, worldPos);

    vertexOut.worldPos = worldPos.xyz;
    vertexOut.worldNormal = normalize(mul(pushConstant.modelMatrix, float4(vertexIn.normal, 0.f))).xyz;
    vertexOut.texCoord = vertexIn.texCoord;
}