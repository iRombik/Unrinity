#pragma once
#include "common.fx"

float3 GetPositionFromDepth(float2 texCoord, float z)
{
    // Get x/w and y/w from the viewport position
    float x = texCoord.x * 2.f - 1.f;
    float y = (1.f - texCoord.y) * 2.f - 1.f;
    float4 projectedPos = float4(x, y, z, 1.0f);
    // Transform by the inverse projection matrix
    float4 positionVS = mul(projInv, projectedPos);
    // Divide by w to get the view-space position
    return positionVS.xyz / positionVS.w;
}