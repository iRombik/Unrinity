#include "baseCommon.fx"

[[vk::binding(16)]] Texture2D texAlbedo;
[[vk::binding(17)]] Texture2D texNormal;
[[vk::binding(18)]] Texture2D texDisplacement;
[[vk::binding(19)]] Texture2D texMetalRoughness;

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

float DistributionGGX(float3 N, float3 H, float a)
{
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
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
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

void main(in VERTEX_OUTPUT vertexOut, out PIXEL_OUTPUT pixelOut) {
    float4 albedo = texAlbedo.Sample(anisoSampler, vertexOut.texCoord);
    float3 normal = texNormal.Sample(anisoSampler, vertexOut.texCoord).rgb;
    float displacement = texDisplacement.Sample(anisoSampler, vertexOut.texCoord).r;
    float roughness = texMetalRoughness.Sample(anisoSampler, vertexOut.texCoord).r;
    float3 metalness = texMetalRoughness.Sample(anisoSampler, vertexOut.texCoord).bbb;

    float3 normalDecompressed = normalize(normal * 2.f - 1.f);
    float3 worldNormal = normalize(vertexOut.worldNormal);

    float3 T, B;
    float3x3 TBN = CalculateTBN(vertexOut.worldPos, worldNormal, vertexOut.texCoord, T, B);
    float3 N = TransposeNormal(normalDecompressed, TBN);
    float3 V = normalize(viewPos - vertexOut.worldPos);
	
    float3 F0 = 0.04f; 
    F0 = lerp(F0, albedo.rgb, metalness);

    // reflectance equation
    float3 Lo = 0.0f;
    for(int i = 0; i < 1; ++i) 
    {
        float3 dirToLight = pointLight0.pos - vertexOut.worldPos;
        float distToLight = length(dirToLight);
        //todo: I took it form epic prs notes 2013. Should rewatch it again after HDR implementation
        float falloffParam = distToLight / pointLight0.areaLight;
        float lightFalloff = pow(saturate(1.f - pow(falloffParam, 4.f)), 2.f) / (falloffParam * falloffParam + 1.f);
        float attenuation = clamp(pointLight0.intensity * lightFalloff, 0.f, 64.f);
        float3 L = normalize(dirToLight);

        // calculate per-light radiance
        float3 H = normalize(V + L);
        float3 radiance = pointLight0.color * attenuation;
        
        // cook-torrance brdf
        float  NDF = DistributionGGX(N, H, roughness);
        float  G   = GeometrySmith(N, V, L, GeomDirectLightParam(roughness));
        float3 F   = FresnelSchlick(max(dot(H, V), 0.0f), F0);
        
        float3 kS = F;
        float3 kD = 1.0f - kS;
        kD *= 1.0f - metalness;	  
        
        float3 numerator  = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
        float3 specular   = numerator / max(denominator, 0.001f);  
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo.rgb / M_PI + specular) * radiance * NdotL;
        Lo += albedo.rgb * ambientColor;
    }   

    float3 color = Lo;
    pixelOut.outColor = float4(color, 1.0f);
}