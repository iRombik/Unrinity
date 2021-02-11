#include "common.fx"
#include "commonFunctions.fx"
#include "terrainCommon.fx"

void main(in VERTEX_OUTPUT vertexOut, out float4 outColor : SV_Target) {
    outColor = float4(0.f, 1.f, 0.f, 1.f);
}