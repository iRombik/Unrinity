#pragma once
#define M_PI 3.14159265358979f

struct POINT_LIGHT {
    float3 pos;
    float  areaLight;
    float3 color;
    float  intensity;
};

struct DIRECTIONAL_LIGHT{
    float3 pos;
    float  attenuationParam;
    float3 color;
    float  intensity;
    float3 direction;
};

[[vk::binding(0)]]
cbuffer COMMON_BUFFER : register(b0) 
{
    float3   viewPos;
    float    curTime;
    float4x4 worldViewProj;
};

[[vk::binding(1)]]
cbuffer LIGHT_BUFFER : register (b1) {
    DIRECTIONAL_LIGHT dirLight;
    POINT_LIGHT pointLight0;
    float4x4 dirLightViewProj;
    float4x4 pointLightViewProj;
    float3   ambientColor;
};

[[vk::binding(3)]]
cbuffer MATERIAL_BUFFER : register (b3) {
    float3 materialDbgMetalness;
    float  materialDbgRoughness;
};

//common textures
[[vk::binding(30)]] Texture2D texShadowMap;

//common samplers
[[vk::binding(120)]] SamplerState anisoSampler;
[[vk::binding(121)]] SamplerState linearSampler;
[[vk::binding(122)]] SamplerState linearClampSampler;
[[vk::binding(123)]] SamplerState pointClampSampler;
[[vk::binding(124)]] SamplerComparisonState cmpPointClampSampler;

static const float4x4 projToScreenMat = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
