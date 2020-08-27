#include "common.fx"

[[vk::binding(16)]] Texture2D fontTexture;

struct VERTEX_OUTPUT {
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

void main(in VERTEX_OUTPUT vertexOutput, out float4 outColor : SV_Target)
{
    outColor = vertexOutput.color * fontTexture.Sample(linearSampler, vertexOutput.uv);
}
