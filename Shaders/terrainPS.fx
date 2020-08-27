#include "common.fx"
#include "commonFunctions.fx"
#include "terrainCommon.fx"

void main(in VERTEX_OUTPUT vertexOut, out float4 outColor : SV_Target) {
    float shadowFactor = ShadowFactorPCF(vertexOut.worldPos);
    outColor = shadowFactor;
}