#include "common.fx"
[[vk::binding(20)]]  Texture2D texSource;

void main(in float2 texCoord : TEXCOORD0, out float4 outColor : SV_Target)
{
    outColor = texSource.Sample(linearClampSampler, texCoord);
}
