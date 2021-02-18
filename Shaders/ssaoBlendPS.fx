#include "ssaoCommon.fx"
#include "commonFunctions.fx"

[[vk::binding(30)]] Texture2D texSSAOMask;

void main(in float2 texCoord : TEXCOORD0, out float pixelOut : SV_Target)
{
    float2 texelSize = 1.0f / float2(1600.f, 900.f);

    float ssaoFactor = 0.f;
    for (int i = -2; i < 2; i++) {
        for (int j = -2; j < 2; j++) {
            float ssaoMask = texSSAOMask.Sample(pointSampler, texCoord + float2(i,j) * texelSize).r;
            ssaoFactor += ssaoMask;
        }
    }
    pixelOut = ssaoFactor / 16.f;
}
