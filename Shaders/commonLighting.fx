#pragma once
#include "common.fx"

float ShadowFactorPCF(Texture2D texShadowMap, float4 worldPos) {
    float4 dirLightSmPos = mul(projToScreenMat, mul(dirLightViewProj, worldPos));
    float2 dirLightSmUV = dirLightSmPos.xy / dirLightSmPos.w;
    if (any(dirLightSmUV < 0.f) || any(dirLightSmUV > 1.f)) {
        return 1.f;
    }
    float  dirLightDepth = dirLightSmPos.z / dirLightSmPos.w;
/*
    float center = 0.6f;
    float cross = 0.8f;
    float diag = 0.6f;

    float3x3 coefs = {
        {diag, cross, diag},
        {cross, center, cross},
        {diag, cross, diag}
    };
*/

    float shadowParam = 0.f;
    float count = 0.f;
    float d = 1.f / 2024.f;
    if (debugDrawMode == 1) {
        float2 offset = (frac(dirLightSmPos.xy * 0.5) > 0.25);  // mod 
        offset.y += offset.x;  
        // y ^= x in floating point 
        if (offset.y > 1.1)   offset.y = 0;
        shadowParam = texShadowMap.SampleCmpLevelZero(cmpLinearClampSampler, dirLightSmUV + d * (offset + float2(-1.5, 0.5)), dirLightDepth).r;;
        shadowParam+= texShadowMap.SampleCmpLevelZero(cmpLinearClampSampler, dirLightSmUV + d * (offset + float2( 0.5, 0.5)), dirLightDepth).r;;
        shadowParam+= texShadowMap.SampleCmpLevelZero(cmpLinearClampSampler, dirLightSmUV + d * (offset + float2(-1.5,-1.5)), dirLightDepth).r;;
        shadowParam+= texShadowMap.SampleCmpLevelZero(cmpLinearClampSampler, dirLightSmUV + d * (offset + float2( 0.5,-1.5)), dirLightDepth).r;;
        count = 4.f;
    } else if (debugDrawMode == 0) {
        float fromTo = 1.5f;
        for (float i = -fromTo; i <= fromTo; i += 1.f) {
            for (float j = -fromTo; j <= fromTo; j += 1.f) {
                float2 offset = float2(i * d, j * d);
                shadowParam += texShadowMap.SampleCmpLevelZero(cmpLinearClampSampler, dirLightSmUV + offset, dirLightDepth).r;
                count += 1.f;
            }
        
        }
    }

    return shadowParam / count;
}
