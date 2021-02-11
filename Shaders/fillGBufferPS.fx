#include "fillGBufferCommon.fx"

[[vk::binding(20)]] Texture2D texAlbedo;
[[vk::binding(21)]] Texture2D texNormal;
[[vk::binding(22)]] Texture2D texMetalRoughness;

//http://www.thetenthplanet.de/archives/1180
float3x3 CalculateTBN(float3 pos, float3 N, float2 uv, inout float3 T, inout float3 B) {
    const float3 dxPos = ddx(pos);
    const float3 dyPos = ddy(pos);
    const float2 dxUV = ddx(uv);
    const float2 dyUV = ddy(uv);

    const float3 xPerp = cross(N, dxPos);
    const float3 yPerp = cross(dyPos, N);
    
    T = dxUV.x * yPerp + dyUV.x * xPerp;
    B = dxUV.y * yPerp + dyUV.y * xPerp;
    
    //T = normalize(T - N * dot(T, N));
    //B = normalize(cross(N, T));
    B = -normalize(B - N * dot(B, N));
    T = normalize(cross(B, N));
    return float3x3( T, B , N );
}

float3 TransposeNormal(float3 localNormal, float3x3 TBN) {
    return normalize(mul(localNormal, TBN));
}

void main(in VERTEX_OUTPUT vertexOut, out GBUFFER_OUTPUT pixelOut) {
    float4 albedo = texAlbedo.Sample(anisoSampler, vertexOut.texCoord);
    float3 normal = texNormal.Sample(anisoSampler, vertexOut.texCoord).rgb;
    //float displacement = texDisplacement.Sample(anisoSampler, vertexOut.texCoord).r;
    float roughness = texMetalRoughness.Sample(anisoSampler, vertexOut.texCoord).g;
    float metalness = texMetalRoughness.Sample(anisoSampler, vertexOut.texCoord).b;

    clip(albedo.a - 0.5f);

    float3 normalDecompressed = normalize(normal * 2.f - 1.f);
    float3 worldNormal = normalize(vertexOut.worldNormal);

    float3 T, B;
    float3x3 TBN = CalculateTBN(vertexOut.worldPos, worldNormal, vertexOut.texCoord, T, B);
    float3 N = TransposeNormal(normalDecompressed, TBN);

    pixelOut.albedo = albedo;
    pixelOut.normal = float4(N, 0.f);
    pixelOut.metalnessRoughness = float4(metalness, roughness, 0.f, 0.f);
    pixelOut.worldPos = float4(vertexOut.worldPos, 1.f);
}