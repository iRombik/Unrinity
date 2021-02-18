#include "ssaoCommon.fx"
#include "commonFunctions.fx"

[[vk::binding(21)]] Texture2D texGBufferNormal;
[[vk::binding(25)]] Texture2D texDepthBuffer;
[[vk::binding(30)]] Texture2D texSSAOKernel;
[[vk::binding(31)]] Texture2D texSSAONoise;

void main(in VERTEX_OUTPUT vertexOut, out PIXEL_OUTPUT pixelOut) 
{
    float2 noiseScale = float2(1600.f / kernelSide, 900.f / kernelSide);
    
    float  depth    = texDepthBuffer.Sample(pointSampler, vertexOut.texCoord).r;
    float3 originView  = GetPositionFromDepth(vertexOut.texCoord, depth);
    float3 normal     = texGBufferNormal.Sample(pointSampler, vertexOut.texCoord).rgb;
    float2 noiseData = texSSAONoise.Sample(pointSampler, vertexOut.texCoord * noiseScale).rg;
    float3 randomVector = normalize(float3(noiseData.x, noiseData.y, 0.f));

    normal = mul(view, float4(normal, 0.f)).xyz;

    float3 tangent = normalize(randomVector - normal * dot(randomVector, normal));
    float3 bitangent = cross(normal, tangent);
    float3x3 TBN = float3x3(tangent, bitangent, normal);

    float occlusion = 0.f;
    for (int i = 0; i < kernelSide; i++)
    {
        for (int j = 0; j < kernelSide; j++)
        {
            float3 kernelVector = texSSAOKernel.Load(int3(i, j, 0)).rgb;
            float3 offsetPosView = originView + radius * mul(kernelVector, TBN);

            float4 offsetPos = mul(proj, float4(offsetPosView, 1.f));    // from view to clip-space
            offsetPos.xyz /= offsetPos.w;               // perspective divide
            offsetPos.xy = offsetPos.xy * 0.5 + 0.5; // transform to range 0.0 - 1.0  
            offsetPos.y = 1.f - offsetPos.y;

            float sampleDepth = texDepthBuffer.Sample(pointSampler, offsetPos.xy).r;

            float3 realPosView = GetPositionFromDepth(offsetPos.xy, sampleDepth);
            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(originView.z - realPosView.z));
            float obscureCoef = (realPosView.z <= (offsetPosView.z + bias) ? 1.f : 0.f);
            occlusion += obscureCoef * rangeCheck;
        }
    }

    pixelOut.ssaoFactor = 1.f - occlusion / kernelSide / kernelSide;
}
