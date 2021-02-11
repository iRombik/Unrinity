#pragma once
#include "common.fx"

float ShadowFactorPCF(Texture2D texShadowMap, float4 worldPos) {
    float4 dirLightSmPos = mul(projToScreenMat, mul(dirLightViewProj, worldPos));
    float2 dirLightSmUV = dirLightSmPos.xy / dirLightSmPos.w;
    if (any(dirLightSmUV < 0.f) || any(dirLightSmUV > 1.f)) {
        return 1.f;
    }
    float  dirLightDepth = dirLightSmPos.z / dirLightSmPos.w;

    float center = 0.4f;
    float cross = 0.3f;
    float diag = 0.2f;

    float3x3 coefs = {
        {diag, cross, diag},
        {cross, center, cross},
        {diag, cross, diag}
    };

    float d = 1.f / 2024.f;
    float shadowParam = 0.f;
    float count = 0.f;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            float2 offset = float2(i * d, j * d);
            shadowParam += coefs[i + 1][j + 1] * texShadowMap.SampleCmpLevelZero(cmpPointClampSampler, dirLightSmUV + offset, dirLightDepth).r;
            count += coefs[i + 1][j + 1];
        }
    }

    return shadowParam / count;
    //Gather
}
