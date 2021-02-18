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
    float4x4 proj;
    float4x4 projInv;
    float4x4 view;
};

[[vk::binding(1)]]
cbuffer LIGHT_BUFFER : register (b1) {
    DIRECTIONAL_LIGHT dirLight;
    POINT_LIGHT pointLight0;
    float4x4 dirLightViewProj;
    float4x4 pointLightViewProj;
    float3   ambientColor;
};

[[vk::binding(2)]]
cbuffer MATERIAL_BUFFER : register (b2) {
};

[[vk::binding(4)]]
cbuffer CUSTOM_BUFFER {
    float4 cb0;
    float4 cb1;
    float4 cb2;
    float4 cb3;
};

[[vk::binding(6)]]
cbuffer UI_BUFFER 
{
    float2 uiScale;
    float2 uiTranslate;
} ;

[[vk::binding(15)]]
cbuffer DEBUG_BUFFER : register (b15) {
    int debugDrawMode;
};

//common samplers
[[vk::binding(120)]] SamplerState pointSampler;
[[vk::binding(121)]] SamplerState linearSampler;
[[vk::binding(122)]] SamplerState anisoSampler;
[[vk::binding(123)]] SamplerState pointClampSampler;
[[vk::binding(124)]] SamplerState linearClampSampler;
[[vk::binding(125)]] SamplerComparisonState cmpPointClampSampler;
[[vk::binding(126)]] SamplerComparisonState cmpLinearClampSampler;

static const float4x4 projToScreenMat = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
