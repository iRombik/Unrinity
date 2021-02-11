#include "common.fx"

[[vk::push_constant]]
struct PUSH_CONSTANT {
    int sampledTexId;
}pushConstant;

[[vk::binding(20)]] Texture2D fontTexture;
[[vk::binding(21)]] Texture2D userTexture;

struct VERTEX_OUTPUT {
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

void main(in VERTEX_OUTPUT vertexOutput, out float4 outColor : SV_Target)
{
    float4 texSample = 0;
    if (pushConstant.sampledTexId == 0) {
        texSample = fontTexture.Sample(linearSampler, vertexOutput.uv);
    }
    if (pushConstant.sampledTexId == 1) {
        texSample = userTexture.Sample(linearSampler, vertexOutput.uv);
        texSample.a = 1.f;
    }
    outColor = vertexOutput.color * texSample;
}
