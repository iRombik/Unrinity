#pragma once
#include "common.fx"

float ShadowFactorPCF(float4 worldPos) {
    float4 dirLightSmPos = mul(projToScreenMat, mul(dirLightViewProj, worldPos));
    float2 dirLightSmUV = dirLightSmPos.xy / dirLightSmPos.w;
    float  dirLightDepth = dirLightSmPos.z / dirLightSmPos.w;
    return texShadowMap.SampleCmpLevelZero(cmpPointClampSampler, dirLightSmUV, dirLightDepth).r;
    //Gather
}
