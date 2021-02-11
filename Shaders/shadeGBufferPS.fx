#include "shadeGBufferCommon.fx"
#include "commonFunctions.fx"
#include "commonLighting.fx"

[[vk::binding(20)]] Texture2D texGBufferAlbedo;
[[vk::binding(21)]] Texture2D texGBufferNormal;
[[vk::binding(22)]] Texture2D texGBufferMetalnessRoughness;
[[vk::binding(23)]] Texture2D texGBufferWorldPos;
[[vk::binding(24)]] Texture2D texShadowMap;
//[[vk::binding(24)]] Texture2D texDepth;

float DistributionGGX(float3 N, float3 H, float a)
{
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom        = M_PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float nom   = NdotV;
    float denom = NdotV * (1.0f - k) + k;
	
    return nom / denom;
}
  
float GeometrySmith(float3 N, float3 V, float3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
	
    return ggx1 * ggx2;
}

float GeomDirectLightParam(float roughness) {
    float t = (roughness + 1.f);
    return t * t / 8.f;
}

float GeomIBLParam(float roughness) {
    return roughness * roughness / 2.f;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    //return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
    //epic games notes 2013
    return F0 + (1.0f - F0) * pow(2.f, (-5.55473f * cosTheta - 6.98316f) * cosTheta);
}

void main(in VERTEX_OUTPUT vertexOut, out PIXEL_OUTPUT pixelOut) {
    float4 albedo = texGBufferAlbedo.Sample(anisoSampler, vertexOut.texCoord);
    float3 worldNormal = texGBufferNormal.Sample(anisoSampler, vertexOut.texCoord).rgb;
    float roughness = texGBufferMetalnessRoughness.Sample(anisoSampler, vertexOut.texCoord).g;
    float3 metalness = texGBufferMetalnessRoughness.Sample(anisoSampler, vertexOut.texCoord).rrr;
    float4 worldPos = texGBufferWorldPos.Sample(anisoSampler, vertexOut.texCoord);

    float3 N = normalize(worldNormal);
    float3 V = normalize(viewPos - worldPos.xyz);
	
    float3 F0 = 0.04f; 
    F0 = lerp(F0, albedo.rgb, metalness);

    // reflectance equation
    float3 Lo = 0.0f;
    
 
    float shadowFactor = ShadowFactorPCF(texShadowMap, worldPos);
    {
        float3 dirToLight = -dirLight.direction;
        float attenuation = shadowFactor;
        float3 L = normalize(dirToLight);

        // calculate per-light radiance
        float3 H = normalize(V + L);
        float3 radiance = dirLight.intensity * dirLight.color * attenuation;

        // cook-torrance brdf
        float  NDF = DistributionGGX(N, H, roughness);
        float  G = GeometrySmith(N, V, L, GeomDirectLightParam(roughness));
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);

        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metalness;

        float3 numerator = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.001f) * max(dot(N, L), 0.001f);
        float3 specular = numerator / max(denominator, 0.001f);

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0f);
        Lo += (kD * albedo.rgb / M_PI + kS * specular)* radiance* NdotL;
    }
    

    Lo += albedo.rgb * ambientColor;

    float3 color = Lo;
    pixelOut.color = float4(color, 1.0f);
}